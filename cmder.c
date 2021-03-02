#include "cmder.h"
#include "estr.h"
#include "cutils.h"
#include <getopt.h>

#include <stdio.h> // remove later

struct cmder_cmd_handle {
	char* name;
    cmder_callback_t callback;
    cmder_handle_t cmder;
    cmder_opt_handle_t* opts;
    size_t opts_len;
    char* getoopts;
};

struct cmder_handle {
    char* name;
    void* context;
    size_t cmdline_max_len;
    cmder_cmd_handle_t* cmds;
    size_t cmds_len;
};

cmder_handle_t cmder_create(cmder_t* config) {
    if(!config)
        return NULL;
    
    cmder_handle_t cmder = cuctor2(cmder_handle_t, struct cmder_handle,
        .name = strdup(config->name),
        .context = config->context,
        .cmdline_max_len = config->cmdline_max_len > 0 ? config->cmdline_max_len : CMDER_DEFAULT_CMDLINE_MAX_LEN
    );

    // memcheck?

    return cmder;
}

char* cmder_getoopts(cmder_cmd_handle_t cmd) {
    if(!cmd || cmd->opts_len <= 0)
        return NULL;

    size_t len = 1; // :

    for(size_t i = 0; i < cmd->opts_len; i++) {
        cmder_opt_handle_t opt = cmd->opts[i];
        len += opt->is_arg ? (opt->is_optional ? 3 : 2) : 1;
    }

    char* getoopts = malloc(len + 1);

    if(!getoopts)
        return NULL;

    getoopts[0] = ':';
    char* ptr = getoopts + 1;

    for(size_t i = 0; i < cmd->opts_len; i++) {
        cmder_opt_handle_t opt = cmd->opts[i];
        *ptr++ = opt->name;
        if(!opt->is_arg) continue;
        *ptr++ = ':';
        if(opt->is_optional) *ptr++ = ':';
    }

    getoopts[len] = '\0';

    return getoopts;
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
    argv[i] = strndup(start, end - start); /* memcheck? */ \
    if(strstr(argv[i], "\\\"")) { \
        char* tmp = estrrep(argv[i], "\\\"", "\""); \
        if(tmp) { \
            free(argv[i]); \
            argv[i] = tmp; \
        } \
    } \
    i++; \
}

char** cmder_argv(const char* cmdline, int* argc) {
    if(!argc)
        return NULL;
    
    if(!cmdline) {
        *argc = 0;
        return NULL;
    }

    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len <= 0) {
        *argc = 0;
        return NULL;
    }

    char* curr = (char*) cmdline;
    char* prev = NULL;
    char* start = NULL;
    bool quoted = false;
    bool quote_escaped = false;
    int len = 0;

    while(*curr) cmder_argv_iteration({ len++; });

    if(quoted) { // last quote not closed
        *argc = 0;
        return NULL;
    }

    cmder_argv_post_iteration_pickup({ len++; });

    if(len <= 0 || quoted) {
        *argc = 0;
        return NULL;
    }

    curr = (char*) cmdline;
    prev = start = NULL;
    quoted = quote_escaped = false;
    char* end = NULL;
    int i = 0;

    char** argv = calloc(len, len * sizeof(char*));

    // memcheck?

    while(*curr) cmder_argv_iteration({
        cmder_argv_iteration_handler();
    });

    cmder_argv_post_iteration_pickup({
        cmder_argv_iteration_handler();
    });
    
    *argc = len;
    return argv;
}

static cu_err_t getoopts_recalc(cmder_cmd_handle_t cmd) {
    char* getoopts = cmder_getoopts(cmd);

    if(!getoopts)
        return CU_ERR_CMDER_GETOOPTS_CALC_FAIL;
    
    free(cmd->getoopts);
    cmd->getoopts = getoopts;

    return CU_OK;
}

static cmder_cmd_handle_t get_cmd_by_name(cmder_handle_t cmder, const char* cmd_name) {
    if(!cmder)
        return NULL;

    for(size_t i = 0; i < cmder->cmds_len; i++) {
        if(estreq(cmd_name, cmder->cmds[i]->name)) {
            return cmder->cmds[i];
        }
    }

    return NULL;
}

cu_err_t cmder_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd, cmder_cmd_handle_t* out_cmd) {
    if(!cmder || !cmd)
        return CU_ERR_INVALID_ARG;

    if(!cmd->callback) // no callback, no need to register cmd
        return CU_ERR_INVALID_ARG;

    if(get_cmd_by_name(cmder, cmd->name)) // already exist
        return CU_ERR_CMDER_CMD_EXIST;
    
    cmder_cmd_handle_t _cmd = cuctor2(cmder_cmd_handle_t, struct cmder_cmd_handle,
        .cmder = cmder,
        .name = strdup(cmd->name),
        .callback = cmd->callback
    );

    // memcheck?

    cmder_cmd_handle_t* cmds = realloc(cmder->cmds, ++cmder->cmds_len * sizeof(cmder_cmd_handle_t));

    // memcheck?

    cmder->cmds = cmds;
    cmder->cmds[cmder->cmds_len - 1] = _cmd;

    if(out_cmd)
        *out_cmd = _cmd;

    return CU_OK;
}

static cmder_opt_handle_t get_opt_by_name(cmder_cmd_handle_t cmd, char name) {
    if(!cmd)
        return NULL;
    
    for(size_t i = 0; i < cmd->opts_len; i++) {
        if(cmd->opts[i]->name == name) {
            return cmd->opts[i];
        }
    }

    return NULL;
}

static void opt_free(cmder_opt_handle_t opt) {
    if(!opt)
        return;

    free(opt);
}

static void cmd_free(cmder_cmd_handle_t cmd) {
    if(!cmd)
        return;

    cmd->cmder = NULL;
    cmd->callback = NULL;
    free(cmd->name);
    cmd->name = NULL;
    free(cmd->getoopts);
    cmd->getoopts = NULL;
    culist_free_(cmd->opts, cmd->opts_len, opt_free);
    free(cmd);
}

cu_err_t cmder_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt) {
    if(!cmd || !cmd->cmder || !opt)
        return CU_ERR_INVALID_ARG;

    if(get_opt_by_name(cmd, opt->name)) // already exist
        return CU_ERR_CMDER_OPT_EXIST;
    
    cmder_opt_handle_t _opt = cuctor2(cmder_opt_handle_t, cmder_opt_t,
        .name = opt->name,
        .is_arg = opt->is_arg,
        .is_optional = opt->is_optional
    );

    if(!_opt) {
        return CU_ERR_NO_MEM;
    }

    cmder_opt_handle_t* opts = realloc(cmd->opts, ++cmd->opts_len * sizeof(cmder_opt_handle_t));
    
    if(!opts) {
        opt_free(_opt);
        return CU_ERR_NO_MEM;
    }

    cmd->opts = opts;
    cmd->opts[cmd->opts_len - 1] = _opt;

    return getoopts_recalc(cmd);
}

cmder_opt_val_t* cmder_opt_val(char optname, cmder_cmd_val_t* cmdval) {
    cmder_opt_val_t** optvals = cmdval->opts;

    for(size_t i = 0; i < cmdval->opts_len; i++) {
        if(optvals[i]->opt->name == optname) {
            return optvals[i];
        }
    }

    return NULL;
}

static void optval_free(cmder_opt_val_t* optval) {
    if(!optval)
        return;
    
    optval->opt = NULL;
    optval->state = false;
    optval->val = NULL;
    free(optval);
}

static void cmdval_free(cmder_cmd_val_t* cmdval) {
    if(!cmdval)
        return;
    
    cmdval->cmder = NULL;
    cmdval->context = NULL;
    culist_free_(cmdval->opts, cmdval->opts_len, optval_free);
    free(cmdval);
}

static bool mandatory_opts_are_set(cmder_opt_val_t** optvals, size_t len) {
    cmder_opt_handle_t opt;

    for(size_t i = 0; i < len; i++) {
        opt = optvals[i]->opt;

        if(opt->is_arg && !opt->is_optional && !optvals[i]->val) {
            return false;
        }
    }

    return true;
}

cu_err_t cmder_run(cmder_handle_t cmder, const char* cmdline) {
    if(!cmder || !cmdline)
        return CU_ERR_INVALID_ARG;

    if(cmder->cmds_len <= 0) // no registered cmds
        return CU_ERR_CMDER_NO_CMDS;

    size_t cmdline_len = strlen(cmdline);

    if(cmdline_len > cmder->cmdline_max_len)
        return CU_ERR_CMDER_CMDLINE_TOO_BIG;
    
    size_t cmder_name_len = strlen(cmder->name);

    if(!estrneq(cmdline, cmder->name, cmder_name_len)) // not for us
        return CU_ERR_CMDER_IGNORE;

    int argc;
    char** argv = cmder_argv(cmdline + cmder_name_len, &argc);

    if(!argv || argc <= 0) // nothing to do
        return CU_ERR_CMDER_IGNORE;

    cmder_cmd_handle_t cmd = get_cmd_by_name(cmder, argv[0]); // argv[0] is cmd name

    if(!cmd) // cmd does not exist
        return CU_ERR_CMDER_CMD_NOEXIST;
    
    cmder_cmd_val_t* cmdval = cuctor(cmder_cmd_val_t,
        .cmder = cmder,
        .cmd = cmd,
        .context = cmder->context
    );

    // memcheck?

    if(cmd->opts_len <= 0) {
        cmd->callback(cmdval);
        cmdval_free(cmdval);
        culist_free(argv, argc);
        return CU_OK;
    }

    cmdval->opts_len = cmd->opts_len;
    cmdval->opts = calloc(cmdval->opts_len, sizeof(cmder_opt_val_t*));

    // memcheck?

    for(size_t i = 0; i < cmd->opts_len; i++) {
        cmdval->opts[i] = cuctor(cmder_opt_val_t,
            .opt = cmd->opts[i]
        );

        // memcheck?
    }

    cu_err_t err = CU_OK;

    opterr = 0;
    optind = 0;

    int o;
    while((o = getopt(argc, argv, cmd->getoopts)) != -1) {
        if(o == ':') {
            // value argument missing for option: optopt
            err = CU_ERR_CMDER_OPT_ARG_MISSING;
            break;
        } else if(o == '?') {
            // unknown option: optopt
            err = CU_ERR_CMDER_UNKNOWN_OPTION;
            break;
        } else {
            cmder_opt_val_t* optval = cmder_opt_val(o, cmdval);

            if(!optval) { // interesting, but not found.. this should not happen
                err = CU_FAIL;
                break;
            }

            if(optval->opt->is_arg) {
                optval->val = optarg;
            } else {
                optval->state = true;
            }
        }
    }

    if(err == CU_OK) {
        if(mandatory_opts_are_set(cmdval->opts, cmdval->opts_len)) {
            for(; optind < argc; optind++) {
                printf("cmd %s has extra argument: %s\n", cmd->name, argv[optind]);
            }

            cmd->callback(cmdval);
        } else {
            err = CU_ERR_CMDER_ARGS_NOT_SET;
        }
    }

    cmdval_free(cmdval);
    culist_free(argv, argc);

    return err;
}

void cmder_destroy(cmder_handle_t cmder) {
    if(!cmder)
        return;

    culist_free_(cmder->cmds, cmder->cmds_len, cmd_free);
    free(cmder->name);
    cmder->name = NULL;
    cmder->context = NULL;
    free(cmder);
}