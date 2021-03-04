#include "cmder.h"
#include "estr.h"
#include "cutils.h"
#include <getopt.h>

struct cmder_cmd_handle {
	char* name;
    cmder_callback_t callback;
    cmder_handle_t cmder;
    cmder_opt_handle_t* opts;
    uint16_t opts_len;
    char* getoopts;
};

struct cmder_handle {
    char* name;
    void* context;
    size_t cmdline_max_len;
    cmder_cmd_handle_t* cmds;
    uint16_t cmds_len;
};

cu_err_t cmder_create(cmder_t* config, cmder_handle_t* out_handle) {
    if(!config || !out_handle)
        return CU_ERR_INVALID_ARG;

    char* _name = strdup(config->name);

    if(!_name) {
        return CU_ERR_NO_MEM;
    }
    
    cmder_handle_t cmder = cu_tctor(cmder_handle_t, struct cmder_handle,
        .name = _name,
        .context = config->context,
        .cmdline_max_len = config->cmdline_max_len > 0 ? config->cmdline_max_len : CMDER_DEFAULT_CMDLINE_MAX_LEN
    );

    if(!cmder) {
        free(_name);
        return CU_ERR_NO_MEM;
    }

    *out_handle = cmder;

    return CU_OK;
}

cu_err_t cmder_getoopts(cmder_cmd_handle_t cmd, char** out_getoopts) {
    if(!cmd || !out_getoopts) {
        return CU_ERR_INVALID_ARG;
    }

    if(cmd->opts_len <= 0) {
        return CU_ERR_CMDER_NO_CMDS;
    }

    uint16_t len = 1;

    for(uint16_t i = 0; i < cmd->opts_len; i++) {
        cmder_opt_handle_t opt = cmd->opts[i];
        len += opt->is_arg ? 2 : 1;
    }

    char* getoopts = malloc(len + 1);

    if(!getoopts) {
        return CU_ERR_NO_MEM;
    }

    getoopts[0] = ':';
    char* ptr = getoopts + 1;

    for(uint16_t i = 0; i < cmd->opts_len; i++) {
        cmder_opt_handle_t opt = cmd->opts[i];
        *ptr++ = opt->name;
        if(opt->is_arg) *ptr++ = ':';
    }

    getoopts[len] = '\0';
    *out_getoopts = getoopts;

    return CU_OK;
}

#define cmder_argv_iteration(CODE) { \
    if(*curr == '"' && (! (quote_escaped = (prev && *prev == '\\')))) quoted = !quoted; \
    else if(!start && (quoted || *curr != ' ')) { \
        start = curr; \
    } else if(start && !quoted && *curr == ' ') { \
        { CODE }; \
        start = NULL; \
    } \
    prev = curr++; \
}

#define cmder_argv_post_iteration_pickup(CODE) {\
    if(start) { \
        { CODE }; \
        start = NULL; \
    } \
}

#define cmder_argv_iteration_handler() { \
    end = *prev == '"' && !quote_escaped ? prev : curr; \
    argv[i] = strndup(start, end - start); \
    if(!argv[i]) { goto _nomem; } \
    if(strstr(argv[i], "\\\"")) { \
        char* tmp = estr_rep(argv[i], "\\\"", "\""); \
        if(tmp) { \
            free(argv[i]); \
            argv[i] = tmp; \
        } else { \
            goto _nomem; \
        } \
    } \
    i++; \
}

cu_err_t cmder_args(const char* cmdline, int* argc, char*** out_argv) {
    if(!cmdline || !argc || !out_argv) {
        return CU_ERR_INVALID_ARG;
    }

    char** argv = NULL;
    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    char* curr = (char*) cmdline;
    char* prev = NULL;
    char* start = NULL;
    bool quoted = false;
    bool quote_escaped = false;
    int len = 0;

    while(*curr) cmder_argv_iteration({ len++; });

    if(quoted) { // last quote not closed
        return CU_ERR_SYNTAX_ERROR;
    }

    cmder_argv_post_iteration_pickup({ len++; });

    if(len <= 0 || quoted) {
        return CU_FAIL;
    }

    curr = (char*) cmdline;
    prev = start = NULL;
    quoted = quote_escaped = false;
    char* end = NULL;
    int i = 0;

    cu_err_t err = CU_OK;
    argv = calloc(len, len * sizeof(char*));

    if(!argv) {
        goto _nomem;
    }

    while(*curr) cmder_argv_iteration({
        cmder_argv_iteration_handler();
    });

    cmder_argv_post_iteration_pickup({
        cmder_argv_iteration_handler();
    });
    
    *argc = len;
    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
    *argc = 0;
    cu_list_free(argv, len);
_return:
    *out_argv = argv;
    return err;
}

static cu_err_t _getoopts_recalc(cmder_cmd_handle_t cmd) {
    char* getoopts = NULL;
    cu_err_t err = cmder_getoopts(cmd, &getoopts);

    if(err != CU_OK) {
        return err;
    }
    
    free(cmd->getoopts); // free old
    cmd->getoopts = getoopts;

    return CU_OK;
}

cu_err_t cmder_get_cmd_by_name(cmder_handle_t cmder, const char* cmd_name, cmder_cmd_handle_t* out_cmd_handle) {
    if(!cmder || !cmd_name) {
        return CU_ERR_INVALID_ARG;
    }

    if(strlen(cmd_name) <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    for(uint16_t i = 0; i < cmder->cmds_len; i++) {
        if(estr_eq(cmd_name, cmder->cmds[i]->name)) {
            if(out_cmd_handle) {
                *out_cmd_handle = cmder->cmds[i];
            }
            
            return CU_OK;
        }
    }

    return CU_ERR_NOT_FOUND;
}

static void _opt_free(cmder_opt_handle_t opt) {
    if(!opt)
        return;

    free(opt);
}

static void _cmd_free(cmder_cmd_handle_t cmd) {
    if(!cmd)
        return;

    cmd->cmder = NULL;
    cmd->callback = NULL;
    free(cmd->name);
    cmd->name = NULL;
    free(cmd->getoopts);
    cmd->getoopts = NULL;
    cu_list_tfreex(cmd->opts, uint16_t, cmd->opts_len, _opt_free);
    free(cmd);
}

cu_err_t cmder_add_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd, cmder_cmd_handle_t* out_cmd) {
    if(!cmder || !cmd)
        return CU_ERR_INVALID_ARG;

    if(!cmd->callback) // no callback, no need to register cmd
        return CU_ERR_INVALID_ARG;
    
    if(cmder_get_cmd_by_name(cmder, cmd->name, NULL) == CU_OK) // already exist
        return CU_ERR_CMDER_CMD_EXIST;
    
    char* _name = strdup(cmd->name);

    if(!_name) {
        return CU_ERR_NO_MEM;
    }

    cmder_cmd_handle_t _cmd = cu_tctor(cmder_cmd_handle_t, struct cmder_cmd_handle,
        .cmder = cmder,
        .name = _name,
        .callback = cmd->callback
    );

    if(!_cmd) {
        free(_name);
        return CU_ERR_NO_MEM;
    }

    cmder_cmd_handle_t* cmds = realloc(cmder->cmds, ++cmder->cmds_len * sizeof(cmder_cmd_handle_t));

    if(!cmds) {
        _cmd_free(_cmd);
        return CU_ERR_NO_MEM;
    }

    cmder->cmds = cmds;
    cmder->cmds[cmder->cmds_len - 1] = _cmd;

    if(out_cmd) {
        *out_cmd = _cmd;
    }

    return CU_OK;
}

cu_err_t cmder_add_vcmd(cmder_handle_t cmder, cmder_cmd_t* cmd) {
    return cmder_add_cmd(cmder, cmd, NULL);
}

static cmder_opt_handle_t _get_opt_by_name(cmder_cmd_handle_t cmd, char name) {
    if(!cmd)
        return NULL;
    
    for(uint16_t i = 0; i < cmd->opts_len; i++) {
        if(cmd->opts[i]->name == name) {
            return cmd->opts[i];
        }
    }

    return NULL;
}

cu_err_t cmder_add_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt, cmder_opt_handle_t* out_opt) {
    if(!cmd || !cmd->cmder || !opt)
        return CU_ERR_INVALID_ARG;

    if(!estr_is_alnum(opt->name)) {
        return CU_ERR_CMDER_INVALID_OPT_NAME;
    }

    if(_get_opt_by_name(cmd, opt->name)) // already exist
        return CU_ERR_CMDER_OPT_EXIST;
    
    cmder_opt_handle_t _opt = cu_tctor(cmder_opt_handle_t, cmder_opt_t,
        .name = opt->name,
        .is_arg = opt->is_arg,
        .is_optional = opt->is_optional
    );

    if(!_opt) {
        return CU_ERR_NO_MEM;
    }

    cmder_opt_handle_t* opts = realloc(cmd->opts, ++cmd->opts_len * sizeof(cmder_opt_handle_t));
    
    if(!opts) {
        _opt_free(_opt);
        return CU_ERR_NO_MEM;
    }

    cmd->opts = opts;
    cmd->opts[cmd->opts_len - 1] = _opt;

    if(out_opt) {
        *out_opt = _opt;
    }

    return _getoopts_recalc(cmd);
}

cu_err_t cmder_add_vopt(cmder_cmd_handle_t cmd, cmder_opt_t* opt) {
    return cmder_add_opt(cmd, opt, NULL);
}

cu_err_t cmder_get_optval(cmder_cmdval_t* cmdval, char optname, cmder_optval_t** out_optval) {
    if(!cmdval) {
        return CU_ERR_INVALID_ARG;
    }

    cmder_optval_t** optvals = cmdval->optvals;

    if(!optvals || cmdval->optvals_len <= 0) {
        return CU_ERR_CMDER_NO_OPTVALS;
    }

    for(uint16_t i = 0; i < cmdval->optvals_len; i++) {
        if(optvals[i]->opt->name == optname) {
            if(out_optval) {
                *out_optval = optvals[i];
            }

            return CU_OK;
        }
    }

    return CU_ERR_NOT_FOUND;
}

static void _optval_free(cmder_optval_t* optval) {
    if(!optval)
        return;
    
    optval->opt = NULL;
    optval->state = false;
    optval->val = NULL; // no need to free. it's reference to argv item
    free(optval);
}

static void _cmdval_free(cmder_cmdval_t* cmdval) {
    if(!cmdval)
        return;
    
    cmdval->cmder = NULL;
    cmdval->context = NULL;
    cu_list_tfreex(cmdval->optvals, uint16_t, cmdval->optvals_len, _optval_free);
    cmdval->extra_args = NULL; // don't free
    cmdval->extra_args_len = 0;
    free(cmdval);
}

static cmder_opt_handle_t _first_mandatory_opt_which_is_not_set(cmder_optval_t** optvals, uint16_t len) {
    if(!optvals || len == 0) {
        return NULL;
    }

    cmder_opt_handle_t opt;

    for(uint16_t i = 0; i < len; i++) {
        opt = optvals[i]->opt;

        if(opt->is_arg && !opt->is_optional && !optvals[i]->val) {
            return opt;
        }
    }

    return NULL;
}

static cu_err_t _argv_is_in_good_condition(int argc, char** argv, bool* condition) {
    if(argc <= 0 || !argv || !condition) {
        return CU_ERR_INVALID_ARG;
    }

    *condition = true;

    for(int i = 0; i < argc; i++) {
        if(!estr_is_trimmed(argv[i]) || estr_contains_unescaped_chr(argv[i], '\"')) {
            *condition = false;
            return CU_ERR_SYNTAX_ERROR;
        }
    }

    return CU_OK;
}

static cu_err_t _cmder_run_args(cmder_handle_t cmder, int argc, char** argv, const void* run_context, bool safe) {
    if(!cmder) {
        return CU_ERR_INVALID_ARG;
    }
    
    if(!argv || argc <= 0) { // nothing to do
        return CU_ERR_CMDER_IGNORE;
    }

    cu_err_t err = CU_OK;
    bool argv_condition;

    if(safe && ((err = _argv_is_in_good_condition(argc, argv, &argv_condition)) != CU_OK || !argv_condition)) {
        return err;
    }

    cmder_cmd_handle_t cmd = NULL;

    if(cmder_get_cmd_by_name(cmder, argv[0], &cmd) != CU_OK) { // argv[0] is cmd name
        return CU_ERR_CMDER_CMD_NOEXIST;
    }
    
    cmder_cmdval_t* cmdval = cu_ctor(cmder_cmdval_t,
        .cmder = cmder,
        .cmd = cmd,
        .context = cmder->context,
        .run_context = run_context,
        .error = CMDER_CMDVAL_NO_ERROR
    );

    if(!cmdval) {
        goto _nomem;
    }

    if(cmd->opts_len > 0) {
        // make one optval in cmdval for every opt in cmd
        cmdval->optvals_len = cmd->opts_len;
        cmdval->optvals = calloc(cmdval->optvals_len, sizeof(cmder_optval_t*));

        if(!cmdval->optvals) {
            goto _nomem;
        }

        for(uint16_t i = 0; i < cmd->opts_len; i++) {
            cmdval->optvals[i] = cu_ctor(cmder_optval_t,
                .opt = cmd->opts[i]
            );

            if(!cmdval->optvals[i]) {
                goto _nomem;
            }
        }
    }

    opterr = 0;
    optind = 0;

    cmder_optval_t* optval = NULL;
    int o;
    while((o = getopt(argc, argv, (!cmd->getoopts ? "" : cmd->getoopts))) != -1) {
        if(o == ':') {
            if(cmder_get_optval(cmdval, optopt, &optval) != CU_OK) {
                err = CU_FAIL;
                goto _return;
            } else if(!optval->opt->is_optional) {
                err = CU_ERR_CMDER_OPT_VAL_MISSING;
                cmdval->error = CMDER_CMDVAL_OPTION_VALUE_MISSING;
                cmdval->error_option_name = optopt;
                goto _error;
            }
        } else if(o == '?') {
            err = CU_ERR_CMDER_UNKNOWN_OPTION;
            cmdval->error = CMDER_CMDVAL_UNKNOWN_OPTION;
            cmdval->error_option_name = optopt;
            goto _error;
        } else {
            if(cmder_get_optval(cmdval, o, &optval) != CU_OK) {
                err = CU_FAIL;
                goto _return;
            }

            if(optval->opt->is_arg) {
                optval->val = optarg;
            } else {
                optval->state = true;
            }
        }
    }

    cmder_opt_handle_t notset_opt = _first_mandatory_opt_which_is_not_set(cmdval->optvals, cmdval->optvals_len);

    if(notset_opt) {
        err = CU_ERR_CMDER_OPT_VAL_MISSING;
        cmdval->error = CMDER_CMDVAL_OPTION_VALUE_MISSING;
        cmdval->error_option_name = notset_opt->name;
        goto _error;
    }

    cmdval->extra_args_len = 0;
    for(int i = optind; i < argc; i++, cmdval->extra_args_len++);

    if(cmdval->extra_args_len > 0) {
        cmdval->extra_args = calloc(cmdval->extra_args_len, sizeof(char*));

        if(!cmdval->extra_args) {
            goto _nomem;
        }

        for(int i = optind, j = 0; i < argc; i++, j++) {
            cmdval->extra_args[j] = argv[i];
        }
    }

    cmd->callback(cmdval);
    goto _return;
_error:
    cu_list_tfreex(cmdval->optvals, uint16_t, cmdval->optvals_len, _optval_free);
    cmdval->extra_args = NULL; // don't free
    cmdval->extra_args_len = 0;
    cmd->callback(cmdval);
    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
_return:
    _cmdval_free(cmdval);
    return err;
}

cu_err_t cmder_run_args(cmder_handle_t cmder, int argc, char** argv, const void* run_context) {
    return _cmder_run_args(cmder, argc, argv, run_context, true);
}

cu_err_t cmder_vrun_args(cmder_handle_t cmder, int argc, char** argv) {
    return cmder_run_args(cmder, argc, argv, NULL);
}

cu_err_t cmder_run(cmder_handle_t cmder, const char* cmdline, const void* run_context) {
    if(!cmder || !cmdline)
        return CU_ERR_INVALID_ARG;

    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len <= 0) {
        return CU_ERR_EMPTY_STRING;
    }

    if(cmder->cmds_len <= 0) // no registered cmds
        return CU_ERR_CMDER_NO_CMDS;

    if(cmdline_len > cmder->cmdline_max_len)
        return CU_ERR_CMDER_CMDLINE_TOO_BIG;
    
    size_t cmder_name_len = strlen(cmder->name);

    if(!estrn_eq(cmdline, cmder->name, cmder_name_len)) // not for us
        return CU_ERR_CMDER_IGNORE;

    int argc;
    char** argv = NULL;
    cu_err_t err = cmder_args(cmdline + cmder_name_len, &argc, &argv);

    if(err != CU_OK) {
        return err;
    }

    err = _cmder_run_args(cmder, argc, argv, run_context, false); // not-safe call (cmder_args is safe)
    cu_list_free(argv, argc);

    return err;
}

cu_err_t cmder_vrun(cmder_handle_t cmder, const char* cmdline) {
    return cmder_run(cmder, cmdline, NULL);
}

cu_err_t cmder_destroy(cmder_handle_t cmder) {
    if(!cmder) {
        return CU_ERR_INVALID_ARG;
    }

    cu_list_tfreex(cmder->cmds, uint16_t, cmder->cmds_len, _cmd_free);
    free(cmder->name);
    cmder->name = NULL;
    cmder->context = NULL;
    free(cmder);

    return CU_OK;
}