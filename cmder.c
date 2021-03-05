#include "cmder.h"
#include "estr.h"
#include "cutils.h"
#include <getopt.h>
#include <assert.h>

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
    bool name_as_cmdline_prefix;
    void* context;
    size_t cmdline_max_len;
    cmder_cmd_handle_t* cmds;
    uint16_t cmds_len;
};

typedef enum {
    CMDER_OPT_FLAG,
    CMDER_OPT_OPTIONAL_ARG,
    CMDER_OPT_MANDATORY_ARG
} cmder_opt_type_t;

typedef struct {
    cmder_opt_type_t type;
    cmder_opt_handle_t* opts;
    uint16_t len;
} cmder_opts_group_t;

static void _free_opts_group(cmder_opts_group_t* group) {
    if(!group) {
        return;
    }

    free(group->opts); // don't free individual opt
    group->opts = NULL;
    group->len = 0;
    free(group);
}

cu_err_t cmder_create(cmder_t* config, cmder_handle_t* out_handle) {
    if(!config || !out_handle)
        return CU_ERR_INVALID_ARG;

    char* _name = strdup(config->name);

    if(!_name) {
        return CU_ERR_NO_MEM;
    }
    
    cmder_handle_t cmder = cu_tctor(cmder_handle_t, struct cmder_handle,
        .name = _name,
        .name_as_cmdline_prefix = config->name_as_cmdline_prefix,
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
    
    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
    len = 0;
    cu_list_free(argv, len);
_return:
    *argc = len;
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

    free(opt->description);
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
    
    char* _desc = NULL;

    if(opt->description) {
        _desc = strdup(opt->description);

        if(!_desc) {
            return CU_ERR_NO_MEM;
        }
    }

    cmder_opt_handle_t _opt = cu_tctor(cmder_opt_handle_t, cmder_opt_t,
        .name = opt->name,
        .is_arg = opt->is_arg,
        .is_optional = opt->is_optional,
        .description = _desc
    );

    if(!_opt) {
        free(_desc);
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

/**
 * @brief Calculate how much is there options with specific type.
 *        Set param "out_opts" to NULL if you want just to calculate
 *        without memory allocation for options array
 * 
 * @param cmd command
 * @param type option type
 * @param out_opts options array (if NULL - no memory will be alocated. just count mode)
 * @param out_len options array length
 * 
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
static cu_err_t _cmd_options(cmder_cmd_handle_t cmd, cmder_opt_type_t type, cmder_opt_handle_t** out_opts, uint16_t* out_len) {
    if(!cmd) {
        return CU_ERR_INVALID_ARG;
    }

    uint16_t len = 0;

    for(uint16_t i = 0; i < cmd->opts_len; i++) {
        if(cmd->opts[i]->is_arg) {
            if(cmd->opts[i]->is_optional && type == CMDER_OPT_OPTIONAL_ARG) {
                len++;
            } else if(!cmd->opts[i]->is_optional && type == CMDER_OPT_MANDATORY_ARG) {
                len++;
            }
        } else if(type == CMDER_OPT_FLAG) {
            len++;
        }
    }

    if(len == 0 || !out_opts) {
        if(out_opts) { *out_opts = NULL; }
        if(out_len) { *out_len = len; }
        
        return CU_OK;
    }

    cmder_opt_handle_t* opts = NULL;
    cu_mem_checkr(opts = malloc(len * sizeof(cmder_opt_handle_t)));

    for(uint16_t i = 0, j = 0; i < cmd->opts_len; i++) {
        if(cmd->opts[i]->is_arg) {
            if(cmd->opts[i]->is_optional && type == CMDER_OPT_OPTIONAL_ARG) {
                opts[j++] = cmd->opts[i];
            } else if(!cmd->opts[i]->is_optional && type == CMDER_OPT_MANDATORY_ARG) {
                opts[j++] = cmd->opts[i];
            }
        } else if(type == CMDER_OPT_FLAG) {
            opts[j++] = cmd->opts[i];
        }
    }

    *out_opts = opts;

    if(out_len) {
        *out_len = len;
    }

    return CU_OK;
}

static cu_err_t _cmd_create_gopt(cmder_cmd_handle_t cmd, cmder_opt_type_t type, bool just_length, cmder_opts_group_t** out_group) {
    if(!cmd || !out_group) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    cmder_opts_group_t* group = NULL;
    cu_mem_check(group = malloc(sizeof(cmder_opts_group_t)));

    group->type = type;
    cu_err_check(_cmd_options(cmd, type, just_length ? NULL : &group->opts, &group->len));
    
    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
_error:
    _free_opts_group(group);
    group = NULL;
_return:
    *out_group = group;
    return err;
}

static cu_err_t _cmd_gopts(cmder_cmd_handle_t cmd, cmder_opts_group_t*** out_groups, uint16_t* out_len, bool just_lengths) {
    if(! cmd) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    cmder_opts_group_t** groups = NULL;
    uint16_t len = 0;

    if(! cmd->opts || cmd->opts_len == 0) {
        goto _return;
    }

    uint16_t flags = 0, oargs = 0, margs = 0;

    for(uint16_t i = 0; i < cmd->opts_len; i++) {
        if(! cmd->opts[i]->is_arg) {
            flags++;
        } else if(cmd->opts[i]->is_optional) {
            oargs++;
        } else {
            margs++;
        }
    }

    len = (flags > 0 ? 1 : 0) + (oargs > 0 ? 1 : 0) + (margs > 0 ? 1 : 0);

    cu_mem_check(groups = malloc(len * sizeof(cmder_opts_group_t*)));

    uint8_t i = 0;

    if(flags > 0) {
        cu_err_check(_cmd_create_gopt(cmd, CMDER_OPT_FLAG, just_lengths, &groups[i++]));
    }

    if(oargs > 0) {
        cu_err_check(_cmd_create_gopt(cmd, CMDER_OPT_OPTIONAL_ARG, just_lengths, &groups[i++]));
    }

    if(margs > 0) {
        cu_err_check(_cmd_create_gopt(cmd, CMDER_OPT_MANDATORY_ARG, just_lengths, &groups[i++]));
    }

    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
_error:
    cu_list_tfreex(groups, uint16_t, len, _free_opts_group);
_return:
    if(out_len)    { *out_len = len; }
    if(out_groups) { *out_groups = groups; }
    return err;
}

static cu_err_t _calc_gopts_lens(cmder_opts_group_t** groups, uint16_t len, uint16_t* flags, uint16_t* oargs, uint16_t* margs) {
    uint16_t _flags = 0, _oargs = 0, _margs = 0;

    if(groups) {
        for(uint16_t i = 0; i < len; i++) {
            switch(groups[i]->type) {
                case CMDER_OPT_FLAG:
                    _flags = groups[i]->len;
                    break;

                case CMDER_OPT_OPTIONAL_ARG:
                    _oargs = groups[i]->len;
                    break;

                case CMDER_OPT_MANDATORY_ARG:
                    _margs = groups[i]->len;
                    break;
            }
        }
    }

    if(flags) { *flags = _flags; }
    if(oargs) { *oargs = _oargs; }
    if(margs) { *margs = _margs; }

    return CU_OK;
}

/**
 * @param cmd command
 * @param out_len signature length
 * @param out_name_len command name length
 * @param o_gopts options groups
 * @param o_gopts_len options groups length
 * 
 * @return CU_OK on success, otherwise: 
 *         CU_ERR_INVALID_ARG
 */
static cu_err_t _calc_signature_len(cmder_cmd_handle_t cmd, size_t* out_len, size_t* out_name_len, cmder_opts_group_t*** o_gopts, uint16_t* o_gopts_len) {
    if(!cmd || !out_len) {
        return CU_ERR_INVALID_ARG;
    }

    *out_len = 0;

    cu_err_t err = CU_OK;
    size_t name_len = strlen(cmd->name);

    if(out_name_len) { *out_name_len = name_len; }

    if(name_len == 0) {
        return CU_ERR_INVALID_ARG;
    }

    size_t len = name_len;

    uint16_t flags_len = 0, oargs_len = 0, margs_len = 0;
    cmder_opts_group_t** gopts = NULL;
    uint16_t gopts_len = 0;

    if(cmd->opts_len == 0) {
        goto _return;
    }

    cu_err_check(_cmd_gopts(cmd, &gopts, &gopts_len, true));
    cu_err_check(_calc_gopts_lens(gopts, gopts_len, &flags_len, &oargs_len, &margs_len));

    len++; // space

    if(flags_len > 0) {
        len += 9; // "[OPTION] "

        if(flags_len > 1) {
            len += 4; // "... "
        }
    }

    for(uint16_t i = 0; i < oargs_len; i++) {
        len += 10; // "[-x xval] "
    }

    for(uint16_t i = 0; i < margs_len; i++) {
        len += 8; // "-x xval "
    }

    len--; // last space

    
    goto _return;

_error:
    cu_list_tfreex(gopts, uint16_t, gopts_len, _free_opts_group);
_return:
    *out_len = len;
    if(o_gopts)     { *o_gopts = gopts; } else { cu_list_tfreex(gopts, uint16_t, gopts_len, _free_opts_group); }
    if(o_gopts_len) { *o_gopts_len = gopts_len; }
    return err;
}

/**
 * @param cmd command
 * @param out_signature signature (if NULL pointer - new memory will be allocated, 
 *                      otherwise outside buffer will be used)
 * @param out_len signature lenght
 * @param len signature length
 * @param name_length command name length
 * @param flags_len number of flag options
 * @param oargs_len number of optional args
 * @param margs_len number of mandatory args
 * 
 * @return CU_OK on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NO_MEM
 */
static cu_err_t _create_signature(cmder_cmd_handle_t cmd, char** out_signature, size_t* out_len, size_t len, size_t name_len, uint16_t flags_len, uint16_t oargs_len, uint16_t margs_len) {
    if(!cmd || !out_signature || len == 0) {
        return CU_ERR_INVALID_ARG;
    }

    char* signature = NULL;
    char* ptr = NULL;
    cmder_opt_handle_t* opts = NULL;
    cu_err_t err = CU_OK;

    // if outside pointer is not null, that's consider like buffer has been provided
    // and that buffer will be used instead of allocating new memory
    cu_mem_check(signature = *out_signature ? *out_signature : malloc(len + 1));

    ptr = signature;
    memcpy(ptr, cmd->name, name_len);
    ptr += name_len;
    *ptr++ = ' ';

    if(flags_len > 0) {
        memcpy(ptr, "[OPTION] ", 9);
        ptr += 9;

        if(flags_len > 1) {
            memcpy(ptr, "... ", 4);
            ptr += 4;
        }
    }

    if(oargs_len > 0) {
        cu_err_check(_cmd_options(cmd, CMDER_OPT_OPTIONAL_ARG, &opts, NULL));

        for(uint16_t i = 0; i < oargs_len; i++) {
            *ptr++ = '[';
            *ptr++ = '-';
            *ptr++ = opts[i]->name;
            *ptr++ = ' ';
            *ptr++ = opts[i]->name;
            memcpy(ptr, "val] ", 5);
            ptr += 5;
        }

        free(opts);
        opts = NULL;
    }

    if(margs_len > 0) {
        cu_err_check(_cmd_options(cmd, CMDER_OPT_MANDATORY_ARG, &opts, NULL));

        for(uint16_t i = 0; i < margs_len; i++) {
            *ptr++ = '-';
            *ptr++ = opts[i]->name;
            *ptr++ = ' ' ;
            *ptr++ = opts[i]->name;
            memcpy(ptr, "val ", 4);
            ptr += 4;
        }

        free(opts);
        opts = NULL;
    }

    *--ptr = '\0'; // replace last space

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - signature == (long int) len);

    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
_error:
    free(signature);
    signature = NULL;
    len = 0;
_return:
    free(opts);
    opts = NULL;
    *out_signature = signature;
    if(out_len) { *out_len = len; }
    return err;
}

cu_err_t cmder_cmd_signature(cmder_cmd_handle_t cmd, char** out_signature, size_t* out_len) {
    if(!cmd || !out_signature) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err;
    size_t len = 0, name_len = 0;
    cmder_opts_group_t** gopts = NULL;
    uint16_t gopts_len = 0;
    cu_err_check(_calc_signature_len(cmd, &len, &name_len, &gopts, &gopts_len));

    uint16_t flags = 0, oargs = 0, margs = 0;
    cu_err_check(_calc_gopts_lens(gopts, gopts_len, &flags, &oargs, &margs));

    cu_list_tfreex(gopts, uint16_t, gopts_len, _free_opts_group);

    *out_signature = NULL; // new memory will be allocated
    cu_err_check(_create_signature(cmd, out_signature, out_len, len, name_len, flags, oargs, margs));

    goto _return;
_error:
_return:
    cu_list_tfreex(gopts, uint16_t, gopts_len, _free_opts_group);
    return err;
}

static cu_err_t _calc_manual_len(cmder_cmd_handle_t cmd, size_t* out_len, size_t* out_name_len, size_t* out_sig_len, uint16_t* out_flags_len, uint16_t* out_oargs_len, uint16_t* out_margs_len) {
    if(!cmd) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    cmder_opt_handle_t* opts = NULL;
    size_t len = 0, sig_len = 0, name_len = 0;
    cmder_opts_group_t** gopts = NULL;
    uint16_t gopts_len = 0;
    uint16_t flags_len = 0, oargs_len = 0, margs_len = 0;

    cu_err_check(_calc_signature_len(cmd, &sig_len, &name_len, &gopts, &gopts_len));
    cu_err_check(_calc_gopts_lens(gopts, gopts_len, &flags_len, &oargs_len, &margs_len));

    len += 7; // "Usage: "
    len += sig_len;
    len ++; // "\n"

    if(margs_len > 0) {
        cu_err_check(_cmd_options(cmd, CMDER_OPT_MANDATORY_ARG, &opts, NULL));

        len += 16; // "Mandatory args:[NL]"
        for(uint16_t i = 0; i < margs_len; i++) {
            len += 3; // "[TAB]-x"
            if(opts[i]->description) {
                len++; // [SPACE]
                len += strlen(opts[i]->description);
            }
            len++; // "[NL]"
        }

        free(opts);
        opts = NULL;
    }

    if(oargs_len > 0) {
        cu_err_check(_cmd_options(cmd, CMDER_OPT_OPTIONAL_ARG, &opts, NULL));

        len += 15; // "Optional args:[NL]"
        for(uint16_t i = 0; i < oargs_len; i++) {
            len += 3; // "[TAB]-x"
            if(opts[i]->description) {
                len++; // [SPACE]
                len += strlen(opts[i]->description);
            }
            len++; // "[NL]"
        }

        free(opts);
        opts = NULL;
    }

    if(flags_len > 0) {
        cu_err_check(_cmd_options(cmd, CMDER_OPT_FLAG, &opts, NULL));

        len += 9; // "Options:[NL]"
        for(uint16_t i = 0; i < oargs_len; i++) {
            len += 3; // "[TAB]-x"
            if(opts[i]->description) {
                len++; // [SPACE]
                len += strlen(opts[i]->description);
            }
            len++; // "[NL]"
        }

        free(opts);
        opts = NULL;
    }

    if(len > 0) {
        len--; // remove last [NL]
    }

    goto _return;
_error:
_return:
    free(opts);
    opts = NULL;
    cu_list_tfreex(gopts, uint16_t, gopts_len, _free_opts_group);
    if(out_len)       { *out_len = len; }
    if(out_name_len)  { *out_name_len = name_len; }
    if(out_sig_len)   { *out_sig_len = sig_len; }
    if(out_flags_len) { *out_flags_len = flags_len; }
    if(out_oargs_len) { *out_oargs_len = oargs_len; }
    if(out_margs_len) { *out_margs_len = margs_len; }
    return err;
}

static cu_err_t _create_manual(cmder_cmd_handle_t cmd, char** out_manual, size_t* out_len) {
    if(!cmd || !out_manual) {
        return CU_ERR_INVALID_ARG;
    }

    size_t len = 0, name_len = 0, sig_len = 0;
    uint16_t flags_len = 0, oargs_len = 0, margs_len = 0;
    cu_err_t err = _calc_manual_len(cmd, &len, &name_len,  &sig_len, &flags_len, &oargs_len, &margs_len);
    if(err != CU_OK) { return err; }

    cmder_opt_handle_t* opts = NULL;
    char* manual = NULL;
    char* ptr = NULL;

    ptr = manual = malloc(len + 1); // +1 for "\0"
    if(!manual) { goto _nomem; }

    memcpy(ptr, "Usage: ", 7);
    ptr += 7;
    err = _create_signature(cmd, &ptr, NULL, sig_len, name_len, flags_len, oargs_len, margs_len);
    if(err != CU_OK) { if(err == CU_ERR_NO_MEM) { goto _nomem; } else { goto _return; } }

    ptr += sig_len;
    *ptr++ = '\n';

    if(margs_len > 0) {
        err = _cmd_options(cmd, CMDER_OPT_MANDATORY_ARG, &opts, NULL);
        if(err != CU_OK) { if(err == CU_ERR_NO_MEM) { goto _nomem; } else { goto _return; } }

        memcpy(ptr, "Mandatory args:\n", 16);
        ptr += 16;

        for(uint16_t i = 0; i < margs_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = opts[i]->name;
            if(opts[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(opts[i]->description);
                memcpy(ptr, opts[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }

        free(opts);
        opts = NULL;
    }

    if(oargs_len > 0) {
        err = _cmd_options(cmd, CMDER_OPT_OPTIONAL_ARG, &opts, NULL);
        if(err != CU_OK) { if(err == CU_ERR_NO_MEM) { goto _nomem; } else { goto _return; } }

        memcpy(ptr, "Optional args:\n", 15);
        ptr += 15;

        for(uint16_t i = 0; i < oargs_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = opts[i]->name;
            if(opts[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(opts[i]->description);
                memcpy(ptr, opts[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }

        free(opts);
        opts = NULL;
    }

    if(flags_len > 0) {
        err = _cmd_options(cmd, CMDER_OPT_FLAG, &opts, NULL);
        if(err != CU_OK) { if(err == CU_ERR_NO_MEM) { goto _nomem; } else { goto _return; } }

        memcpy(ptr, "Options:\n", 9);
        ptr += 9;

        for(uint16_t i = 0; i < oargs_len; i++) {
            *ptr++ = '\t';
            *ptr++ = '-';
            *ptr++ = opts[i]->name;
            if(opts[i]->description) {
                *ptr++ = ' ';
                size_t _desc_len = strlen(opts[i]->description);
                memcpy(ptr, opts[i]->description, _desc_len);
                ptr += _desc_len;
            }
            *ptr++ = '\n';
        }

        free(opts);
        opts = NULL;
    }

    *--ptr = '\0'; // replace last [NL]

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - manual == (long int) len);

    goto _return;
_nomem:
    err = CU_ERR_NO_MEM;
    free(manual);
    free(opts);
    manual = NULL;
    opts = NULL;
_return:
    *out_manual = manual;
    if(out_len && manual) { *out_len = ptr - manual; }
    return err;
}

cu_err_t cmder_cmd_manual(cmder_cmd_handle_t cmd, char** out_manual, size_t* out_len) {
    return _create_manual(cmd, out_manual, out_len);
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

cu_err_t cmder_cmdval_errstr(cmder_cmdval_t* cmdval, char** out_errstr, size_t* out_len) {
    if(!cmdval || !out_errstr || cmdval->error == CMDER_CMDVAL_NO_ERROR || !cmdval->cmd) {
        return CU_ERR_INVALID_ARG;
    }

    size_t cmdname_len = strlen(cmdval->cmd->name);
    size_t len = cmdname_len;
    len += 2; // ": "

    switch (cmdval->error)
    {
        case CMDER_CMDVAL_UNKNOWN_OPTION:
            len += 7; // "invalid"
            break;
        
        case CMDER_CMDVAL_OPTION_VALUE_MISSING:
            len += 17; // "missing value for"
            break;

        case CMDER_CMDVAL_NO_ERROR:
            // ignore
            break;
    }

    len += 14; // " option -- 'x'"

    char* errorstr = malloc(len + 1);

    if(!errorstr) {
        if(out_len) { *out_len = 0; }
        *out_errstr = NULL;
        return CU_ERR_NO_MEM;
    }

    char* ptr = errorstr;
    memcpy(ptr, cmdval->cmd->name, cmdname_len);
    ptr += cmdname_len;
    *ptr++ = ':';
    *ptr++ = ' ';

    switch (cmdval->error)
    {
        case CMDER_CMDVAL_UNKNOWN_OPTION:
            memcpy(ptr, "invalid", 7);
            ptr += 7;
            break;
        
        case CMDER_CMDVAL_OPTION_VALUE_MISSING:
            memcpy(ptr, "missing value for", 17);
            ptr += 17;
            break;

        case CMDER_CMDVAL_NO_ERROR:
            // ignore
            break;
    }

    memcpy(ptr, " option -- '", 12);
    ptr += 12;
    *ptr++ = cmdval->error_option_name;
    *ptr++ = '\'';
    *ptr = '\0';

    // make sure that ptr is at exact the same place where it should be
    assert(ptr - errorstr == (long int) len);

    *out_errstr = errorstr;
    if(out_len && errorstr) { *out_len = ptr - errorstr; }

    return CU_OK;
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
    cmdval->extra_args = NULL; // don't free()
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

    if(cmder->name_as_cmdline_prefix && !estrn_eq(cmdline, cmder->name, cmder_name_len)) // not for us
        return CU_ERR_CMDER_IGNORE;

    int argc;
    char** argv = NULL;

    cu_err_t err = cmder_args(
        cmder->name_as_cmdline_prefix ? cmdline + cmder_name_len : cmdline,
        &argc,
        &argv
    );

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