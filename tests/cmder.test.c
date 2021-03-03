#include "../cmder.h"
#include "../estr.h"
#include "../cutils.h"
#include <assert.h>

static int data = 1337;
static bool help_fired = false;
static bool echo_fired = false;
static bool ok_fired = false;
char* echo_message;
static cmder_cmd_handle_t ok;

void help_cb(cmder_cmd_val_t* cmdval) {
    help_fired = true;
    assert(cmdval->cmder);
    assert(!cmdval->opts);
    assert(cmdval->opts_len == 0);
    int* ctx = (int*) cmdval->context;
    assert(ctx && ctx == &data && *ctx == data);
    assert(*ctx == 1337);
}

static bool capture_extra_arg0 = false;
static int echo_extra_args_len = 0;
static char* echo_extra_arg0;

void echo_cb(cmder_cmd_val_t* cmdval) {
    echo_fired = true;
    assert(cmdval->cmder);
    assert(cmdval->opts);
    assert(cmdval->opts_len == 1);
    cmder_opt_val_t* optval;
    assert(optval = cmder_get_optval(cmdval, 'm'));
    assert(optval->val);
    assert(!optval->state);
    assert(optval->opt->is_arg);
    assert(!optval->opt->is_optional);
    assert(optval->opt->name == 'm');
    echo_message = strdup(optval->val);
    assert(! cmder_get_optval(cmdval, 'x'));
    echo_extra_args_len = cmdval->extra_args_len;
    if(cmdval->extra_args_len > 0 && capture_extra_arg0) {
        echo_extra_arg0 = strdup(cmdval->extra_args[0]);
    }
}

void ok_cb(cmder_cmd_val_t* cmdval) {
    ok_fired = true;
    assert(cmdval->cmd);
    assert(cmdval->cmd == ok);
}

void null_cb(cmder_cmd_val_t* cmdval) { /* noop */ }

static void test_getoopts(cmder_handle_t cmder);
static void test_args();

int main() {
    test_args();

    cmder_handle_t cmder = cmder_create(&(cmder_t){
        .name = "+esp32",
        .context = &data
    });

    assert(cmder);

    test_getoopts(cmder);

    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    }, NULL) == CU_OK);

    cmder_cmd_handle_t echo;
    
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){
        .name = "echo",
        .callback = &echo_cb
    }, &echo) == CU_OK);

    assert(echo);

    assert(cmder_add_opt(echo, &(cmder_opt_t){
        .name = 'm',
        .is_arg = true
    }) == 0);

    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ 
        .name = "ok",
        .callback = &ok_cb
    }, &ok) == CU_OK);

    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    }, NULL) == CU_ERR_CMDER_CMD_EXIST); // already exist

    assert(cmder_add_opt(echo, &(cmder_opt_t){ .name = 'm' }) == CU_ERR_CMDER_OPT_EXIST); // already exist

    // no callback provided
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "kkk" }, NULL) == CU_ERR_INVALID_ARG);

    assert(cmder_run(cmder, "+") != CU_OK); // wrong cmder name
    assert(cmder_run(cmder, "esp32") != CU_OK); // wrong cmder name
    assert(cmder_run(cmder, "+esp32") != CU_OK); // no cmd name
    assert(cmder_run(cmder, "+esp32 test") != CU_OK); // test cmd does not exists
    assert(cmder_run(cmder, "+esp32 echo") != CU_OK); // missing mandatory option m
    assert(!echo_fired);
    assert(cmder_run(cmder, "+esp32 echo -x") != CU_OK); // unknown option
    assert(!echo_fired);
    assert(cmder_run(cmder, "+esp32 echo -m") != CU_OK); // m is arg and argval missing
    assert(!echo_fired);
    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m hey") == CU_OK); // ok
    assert(echo_fired);
    echo_fired = false;
    assert(estr_eq(echo_message, "hey"));
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    echo_extra_args_len = 0;
    echo_extra_arg0 = NULL;
    capture_extra_arg0 = true;
    assert(cmder_run(cmder, "+esp32 echo -m whats up") == CU_OK); // ok, "up" extra arg
    assert(echo_fired);
    assert(estr_eq(echo_message, "whats"));
    assert(echo_extra_args_len == 1 && echo_extra_arg0);
    assert(estr_eq(echo_extra_arg0, "up"));
    free(echo_extra_arg0);
    echo_extra_args_len = 0;
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -- -m hey") != CU_OK); // parsing terminated with --, missing m
    assert(!echo_fired);
    help_fired = false;
    assert(cmder_run(cmder, "+esp32 help") == CU_OK); // ok
    assert(help_fired);
    ok_fired = false;
    assert(cmder_run(cmder, "+esp32 ok") == CU_OK); // ok
    assert(ok_fired);

    const char* too_long_cmd = "+esp32 echo -m aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    
    // default max is 512, this command is 524
    assert(cmder_run(cmder, too_long_cmd) == CU_ERR_CMDER_CMDLINE_TOO_BIG);

    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m \"whats up\"") == CU_OK); // ok
    assert(echo_fired);
    assert(estr_eq(echo_message, "whats up"));
    free(echo_message);
    echo_message = NULL;

    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m \"this is \\\"quoted\\\" word\"") == CU_OK); // ok
    assert(echo_fired);
    assert(estr_eq(echo_message, "this is \"quoted\" word"));
    free(echo_message);
    echo_message = NULL;

    cmder_destroy(cmder);

    return 0;
}


static void test_getoopts(cmder_handle_t cmder) {
    char* tmp;

    cmder_cmd_handle_t cmplx;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "c1", .callback = &null_cb }, &cmplx) == CU_OK);
    assert(cmplx);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'a' }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'b', .is_arg = true }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'c', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'd' }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'e', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'f', .is_arg = true }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'g' }) == CU_OK);
    assert(cmder_add_opt(cmplx, &(cmder_opt_t){ .name = 'h' }) == CU_OK);
    assert(tmp = cmder_getoopts(cmplx));
    assert(estr_eq(tmp, ":ab:c::de::f:gh"));
    free(tmp);
}

static void test_args() {
    int argc;
    char** argv;
    argv = cmder_args("test a b c", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b") && estr_eq(argv[3], "c"));
    cu_list_free(argv, argc);
    argv = cmder_args("test ab \"c d\" e", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "ab") &&
        estr_eq(argv[2], "c d") && estr_eq(argv[3], "e"));
    cu_list_free(argv, argc);
    argv = cmder_args("test a \"b c\"    \"\"   d", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    argv = cmder_args("  \" a   b c \"  d e \"f\" ", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], " a   b c ") && 
        estr_eq(argv[1], "d") && estr_eq(argv[2], "e") && estr_eq(argv[3], "f"));
    cu_list_free(argv, argc);
    argv = cmder_args("ab \"", &argc);
    assert(!argv && argc == 0); // syntax error
    argv = cmder_args("test a \"b c\"    \"\"   d \"\"", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    argv = cmder_args("test a \"b c\"    \"\"   d \"x\"", &argc);
    assert(argc == 5);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d") && estr_eq(argv[4], "x"));
    cu_list_free(argv, argc);
    argv = cmder_args("\"\"   test a \"b c\"    \"\"   \"d\"  ", &argc);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    argv = cmder_args("a \"b \\\"c\\\" d\"", &argc); // a "b \"c\" d"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "b \"c\" d"));
    cu_list_free(argv, argc);
    argv = cmder_args("a \\\"c\\\"", &argc); // a \"c\"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "\"c\""));
    cu_list_free(argv, argc);
    argv = cmder_args("\\\"a\\\" b", &argc); // \"a\" b
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "\"a\"") && estr_eq(argv[1], "b"));
    cu_list_free(argv, argc);
}