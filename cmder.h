#ifndef _CMDER_H_
#define _CMDER_H

#include <stdlib.h>
#include <stdbool.h>

#define CMDER_DEFAULT_CMDLINE_MAX_LEN 256

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
cmder_cmd_handle_t cmder_cmd(cmder_handle_t cmder, cmder_cmd_t* cmd);
int cmder_opt(cmder_cmd_handle_t cmd, cmder_opt_t* opt);
int cmder_run(cmder_handle_t cmder, const char* cmdline);
cmder_opt_val_t* cmder_opt_val(char optname, cmder_cmd_val_t* cmdval);
void cmder_destroy(cmder_handle_t cmder);

#endif