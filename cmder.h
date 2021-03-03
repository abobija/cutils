#ifndef _CMDER_H_
#define _CMDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include <stdlib.h>
#include <stdbool.h>

#define CMDER_DEFAULT_CMDLINE_MAX_LEN 512

#define CU_ERR_CMDER_BASE                (1000)
#define CU_ERR_CMDER_OPT_EXIST           (CU_ERR_CMDER_BASE + 1)
#define CU_ERR_CMDER_GETOOPTS_CALC_FAIL  (CU_ERR_CMDER_BASE + 2)
#define CU_ERR_CMDER_NO_CMDS             (CU_ERR_CMDER_BASE + 3)
#define CU_ERR_CMDER_CMDLINE_TOO_BIG     (CU_ERR_CMDER_BASE + 4)
#define CU_ERR_CMDER_IGNORE              (CU_ERR_CMDER_BASE + 5)
#define CU_ERR_CMDER_CMD_NOEXIST         (CU_ERR_CMDER_BASE + 6)
#define CU_ERR_CMDER_OPT_ARG_MISSING     (CU_ERR_CMDER_BASE + 7)
#define CU_ERR_CMDER_UNKNOWN_OPTION      (CU_ERR_CMDER_BASE + 8)
#define CU_ERR_CMDER_ARGS_NOT_SET        (CU_ERR_CMDER_BASE + 9)
#define CU_ERR_CMDER_CMD_EXIST           (CU_ERR_CMDER_BASE + 10)

struct cmder_handle;
struct cmder_cmd_handle;
typedef struct cmder_handle* cmder_handle_t;
typedef struct cmder_cmd_handle* cmder_cmd_handle_t;

typedef struct {
    const char* name;
    void* context;
    size_t cmdline_max_len;
} cmder_t;

typedef struct {
    char name;
    bool is_arg;
    bool is_optional;
} cmder_opt_t;

typedef cmder_opt_t* cmder_opt_handle_t;

typedef struct {
    cmder_opt_handle_t opt;
    bool state;
    const char* val;
} cmder_opt_val_t;

typedef struct {
    cmder_handle_t cmder;
    cmder_cmd_handle_t cmd;
    void* context;
    cmder_opt_val_t** opts;
    size_t opts_len;
} cmder_cmd_val_t;

typedef void(*cmder_callback_t)(cmder_cmd_val_t* cmdval);

typedef struct {
    const char* name;
    cmder_callback_t callback;
} cmder_cmd_t;

cmder_handle_t cmder_create(cmder_t* config);
char* cmder_getoopts(cmder_cmd_handle_t cmd);
char** cmder_argv(const char* cmdline, int* argc);
cu_err_t cmder_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd, cmder_cmd_handle_t* out_cmd);
cu_err_t cmder_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt);
cu_err_t cmder_run(cmder_handle_t cmder, const char* cmdline);
cmder_opt_val_t* cmder_opt_val(char optname, cmder_cmd_val_t* cmdval);
void cmder_destroy(cmder_handle_t cmder);

#ifdef __cplusplus
}
#endif

#endif