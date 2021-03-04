#include "../cmder.h"
#include "../estr.h"
#include "../cutils.h"
#include <assert.h>
#include <stdio.h>

static int data = 1337;
static bool help_fired = false;
static bool echo_fired = false;
static bool ok_fired = false;
char* echo_message;
static cmder_cmd_handle_t ok;

void help_cb(cmder_cmdval_t* cmdval) {
    if(cmdval->error != CMDER_CMDVAL_NO_ERROR) {
        return;
    }

    help_fired = true;
    assert(cmdval->cmder);
    assert(!cmdval->optvals);
    assert(cmdval->optvals_len == 0);
    int* ctx = (int*) cmdval->context;
    assert(ctx && ctx == &data && *ctx == data);
    assert(*ctx == 1337);
}

static bool capture_extra_arg0 = false;
static int echo_extra_args_len = 0;
static char* echo_extra_arg0;
static const void* captured_echo_run_context;

void echo_cb(cmder_cmdval_t* cmdval) {
    if(cmdval->error != CMDER_CMDVAL_NO_ERROR) {
        return;
    }

    echo_fired = true;
    captured_echo_run_context = cmdval->run_context;
    assert(cmdval->cmder);
    assert(cmdval->optvals);
    assert(cmdval->optvals_len == 1);
    cmder_optval_t* optval;
    assert(cmder_get_optval(NULL, 'm', &optval) == CU_ERR_INVALID_ARG);
    assert(cmder_get_optval(cmdval, 'm', NULL) == CU_OK);
    assert(cmder_get_optval(cmdval, 'm', &optval) == CU_OK);
    assert(optval->val);
    assert(!optval->state);
    assert(optval->opt->is_arg);
    assert(!optval->opt->is_optional);
    assert(optval->opt->name == 'm');
    echo_message = strdup(optval->val);
    assert(cmder_get_optval(cmdval, 'x', &optval) == CU_ERR_NOT_FOUND);
    assert(cmder_get_optval(cmdval, 'x', NULL) == CU_ERR_NOT_FOUND);
    echo_extra_args_len = cmdval->extra_args_len;
    if(cmdval->extra_args_len > 0 && capture_extra_arg0) {
        echo_extra_arg0 = strdup(cmdval->extra_args[0]);
    }
}

void ok_cb(cmder_cmdval_t* cmdval) {
    if(cmdval->error != CMDER_CMDVAL_NO_ERROR) {
        return;
    }

    ok_fired = true;
    assert(cmdval->cmd);
    assert(cmdval->cmd == ok);
}

void null_cb(cmder_cmdval_t* cmdval) { (void)(cmdval); /* noop */ }

static void test_getoopts(cmder_handle_t cmder);
static void test_args();
static void test_args_ptrs();
static void test_error_callback();
static void test_run_raw_args();
static void test_signatures();
static void test_with_no_prefix();

int main() {
    test_with_no_prefix();
    test_signatures();
    test_run_raw_args();
    test_args();
    test_args_ptrs();
    test_error_callback();

    cmder_handle_t cmder = NULL;

    assert(cmder_create(&(cmder_t){}, NULL) == CU_ERR_INVALID_ARG);
    assert(cmder_create(NULL, &cmder) == CU_ERR_INVALID_ARG);

    assert(cmder_create(&(cmder_t){
        .name = "+esp32",
        .context = &data,
        .name_as_cmdline_prefix = true
    }, &cmder) == CU_OK);

    assert(cmder);

    test_getoopts(cmder);

    assert(cmder_add_vcmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    }) == CU_OK);

    cmder_cmd_handle_t echo;
    
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){
        .name = "echo",
        .callback = &echo_cb
    }, &echo) == CU_OK);

    assert(echo);

    assert(cmder_add_vopt(echo, &(cmder_opt_t){
        .name = '?'
    }) == CU_ERR_CMDER_INVALID_OPT_NAME);

    assert(cmder_add_vopt(echo, &(cmder_opt_t){
        .name = ':'
    }) == CU_ERR_CMDER_INVALID_OPT_NAME);

    assert(cmder_add_vopt(echo, &(cmder_opt_t){
        .name = '+'
    }) == CU_ERR_CMDER_INVALID_OPT_NAME);

    assert(cmder_add_vopt(echo, &(cmder_opt_t){
        .name = 'm',
        .is_arg = true
    }) == CU_OK);

    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ 
        .name = "ok",
        .callback = &ok_cb
    }, &ok) == CU_OK);

    assert(cmder_add_vopt(ok, &(cmder_opt_t){
        .name = '1' // alphanumerics are allowed
    }) == CU_OK);

    assert(cmder_add_vcmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    }) == CU_ERR_CMDER_CMD_EXIST); // already exist

    assert(cmder_add_vopt(echo, &(cmder_opt_t){ .name = 'm' }) == CU_ERR_CMDER_OPT_EXIST); // already exist

    // no callback provided
    assert(cmder_add_vcmd(cmder, &(cmder_cmd_t){ .name = "kkk" }) == CU_ERR_INVALID_ARG);

    assert(cmder_vrun(cmder, "") != CU_OK); // no command
    assert(cmder_vrun(cmder, "+") != CU_OK); // wrong cmder name
    assert(cmder_vrun(cmder, "esp32") != CU_OK); // wrong cmder name
    assert(cmder_vrun(cmder, "+esp32") != CU_OK); // no cmd name
    assert(cmder_vrun(cmder, "+esp32 test") != CU_OK); // test cmd does not exists
    assert(cmder_vrun(cmder, "+esp32 echo") != CU_OK); // missing mandatory option m
    assert(!echo_fired);
    assert(cmder_vrun(cmder, "+esp32 echo -x") != CU_OK); // unknown option
    assert(!echo_fired);
    assert(cmder_vrun(cmder, "+esp32 echo -m") != CU_OK); // m is arg and argval missing
    assert(!echo_fired);
    echo_fired = false;
    assert(cmder_vrun(cmder, "+esp32 echo -m hey") == CU_OK); // ok
    assert(echo_fired);
    echo_fired = false;
    int xx = 123;
    assert(cmder_run(cmder, "+esp32 echo -m hey", &xx) == CU_OK); // ok
    assert(echo_fired);
    assert(captured_echo_run_context);
    assert(*(int*) captured_echo_run_context == xx);
    echo_fired = false;
    assert(estr_eq(echo_message, "hey"));
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    echo_extra_args_len = 0;
    echo_extra_arg0 = NULL;
    capture_extra_arg0 = true;
    assert(cmder_vrun(cmder, "+esp32 echo -m whats up") == CU_OK); // ok, "up" extra arg
    assert(echo_fired);
    assert(estr_eq(echo_message, "whats"));
    assert(echo_extra_args_len == 1 && echo_extra_arg0);
    assert(estr_eq(echo_extra_arg0, "up"));
    free(echo_extra_arg0);
    echo_extra_args_len = 0;
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    assert(cmder_vrun(cmder, "+esp32 echo -- -m hey") != CU_OK); // parsing terminated with --, missing m
    assert(!echo_fired);
    help_fired = false;
    assert(cmder_vrun(cmder, "+esp32 help") == CU_OK); // ok
    assert(help_fired);
    ok_fired = false;
    assert(cmder_vrun(cmder, "+esp32 ok") == CU_OK); // ok
    assert(ok_fired);

    const char* too_long_cmd = "+esp32 echo -m aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    
    // default max is 512, this command is 524
    assert(cmder_vrun(cmder, too_long_cmd) == CU_ERR_CMDER_CMDLINE_TOO_BIG);

    echo_fired = false;
    assert(cmder_vrun(cmder, "+esp32 echo -m \"whats up\"") == CU_OK); // ok
    assert(echo_fired);
    assert(estr_eq(echo_message, "whats up"));
    free(echo_message);
    echo_message = NULL;

    echo_fired = false;
    assert(cmder_vrun(cmder, "+esp32 echo -m \"this is \\\"quoted\\\" word\"") == CU_OK); // ok
    assert(echo_fired);
    assert(estr_eq(echo_message, "this is \"quoted\" word"));
    free(echo_message);
    echo_message = NULL;

    assert(cmder_destroy(NULL) == CU_ERR_INVALID_ARG);
    assert(cmder_destroy(cmder) == CU_OK);

    return 0;
}

static void test_with_no_prefix() {
    cmder_handle_t cmder = NULL;
    assert(cmder_create(&(cmder_t){ .name = "esp" }, &cmder) == CU_OK && cmder);
    cmder_cmd_handle_t cmd = NULL;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "touch", .callback = &null_cb }, &cmd) == CU_OK);
    assert(cmd);

    assert(cmder_vrun(cmder, "esp test") != CU_OK);
    assert(cmder_vrun(cmder, "test") != CU_OK);
    assert(cmder_vrun(cmder, "esp") != CU_OK);
    assert(cmder_vrun(cmder, "esp touch") != CU_OK);
    assert(cmder_vrun(cmder, "touch") == CU_OK);
}

static void test_signatures() {
    cmder_handle_t cmder = NULL;
    assert(cmder_create(&(cmder_t){ .name = "esp" }, &cmder) == CU_OK && cmder);
    cmder_cmd_handle_t cmd = NULL;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "touch", .callback = &null_cb }, &cmd) == CU_OK);
    assert(cmd);

    char* sig = NULL;
    size_t sig_len;
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch") && estr_eq(sig, "touch"));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'a' }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION]") && estr_eq(sig, "touch [OPTION]"));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'f' }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION] ...") && estr_eq(sig, "touch [OPTION] ..."));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'b', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION] ... [-b bval]") 
        && estr_eq(sig, "touch [OPTION] ... [-b bval]"));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'c', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION] ... [-b bval] [-c cval]") 
        && estr_eq(sig, "touch [OPTION] ... [-b bval] [-c cval]"));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'd', .is_arg = true, .is_optional = false }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION] ... [-b bval] [-c cval] -d dval") 
        && estr_eq(sig, "touch [OPTION] ... [-b bval] [-c cval] -d dval"));
    free(sig);

    assert(cmder_add_vopt(cmd, &(cmder_opt_t){ .name = 'e', .is_arg = true, .is_optional = false }) == CU_OK);
    assert(cmder_cmd_signature(cmd, &sig, &sig_len) == CU_OK);
    assert(sig && sig_len == strlen("touch [OPTION] ... [-b bval] [-c cval] -d dval -e eval") 
        && estr_eq(sig, "touch [OPTION] ... [-b bval] [-c cval] -d dval -e eval"));
    free(sig);

    char* manual = NULL;
    size_t manual_len = 0;
    assert(cmder_cmd_manual(cmd, &manual, &manual_len) == CU_OK);
    assert(manual);
    //printf("%ld:\n%s\n", manual_len, manual);
    free(manual);
}

static void test_run_raw_args() {
    cmder_handle_t cmder = NULL;
    assert(cmder_create(&(cmder_t){ .name = "esp" }, &cmder) == CU_OK && cmder);
    assert(cmder_add_vcmd(cmder, &(cmder_cmd_t){ .name = "touch", .callback = &null_cb }) == CU_OK);

    int argc = 0;
    char** argv = NULL;
    argv = calloc(2, sizeof(char*));

    argv[argc++] = strdup("kkk");
    assert(cmder_vrun_args(cmder, argc, argv) == CU_ERR_CMDER_CMD_NOEXIST);

    free(argv[0]);
    argv[0] = strdup("touch");
    assert(cmder_vrun_args(cmder, argc, argv) == CU_OK);

    argv[argc++] = strdup("extra");
    assert(cmder_vrun_args(cmder, argc, argv) == CU_OK);

    free(argv[1]);
    argv[1] = strdup("\"unescaped");
    assert(cmder_vrun_args(cmder, argc, argv) == CU_ERR_SYNTAX_ERROR);

    free(argv[1]);
    argv[1] = strdup(" not trimmed ");
    assert(cmder_vrun_args(cmder, argc, argv) == CU_ERR_SYNTAX_ERROR);
}

static void test_getoopts(cmder_handle_t cmder) {
    char* tmp;

    cmder_cmd_handle_t cmplx;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "c1", .callback = &null_cb }, &cmplx) == CU_OK);
    assert(cmplx);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'a' }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'b', .is_arg = true }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'c', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'd' }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'e', .is_arg = true, .is_optional = true }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'f', .is_arg = true }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'g' }) == CU_OK);
    assert(cmder_add_vopt(cmplx, &(cmder_opt_t){ .name = 'h' }) == CU_OK);
    assert(cmder_getoopts(cmplx, &tmp) == CU_OK);
    assert(estr_eq(tmp, ":ab:c:de:f:gh"));
    free(tmp);
}

static void test_args() {
    int argc;
    char** argv;

    assert(cmder_args(NULL, &argc, &argv) == CU_ERR_INVALID_ARG);
    assert(cmder_args("a b", NULL, &argv) == CU_ERR_INVALID_ARG);
    assert(cmder_args("a b", &argc, NULL) == CU_ERR_INVALID_ARG);
    assert(cmder_args("", &argc, &argv) == CU_ERR_EMPTY_STRING);

    assert(cmder_args("a", &argc, &argv) == CU_OK);
    assert(argc == 1);
    assert(argv && estr_eq(argv[0], "a"));
    cu_list_free(argv, argc);

    assert(cmder_args("ab \"", &argc, &argv) == CU_ERR_SYNTAX_ERROR);
    assert(!argv && argc == 0);

    assert(cmder_args("test a b c", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b") && estr_eq(argv[3], "c"));
    cu_list_free(argv, argc);
    assert(cmder_args("test ab \"c d\" e", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "ab") &&
        estr_eq(argv[2], "c d") && estr_eq(argv[3], "e"));
    cu_list_free(argv, argc);
    assert(cmder_args("test a \"b c\"    \"\"   d", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    assert(cmder_args("  \" a   b c \"  d e \"f\" ", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], " a   b c ") && 
        estr_eq(argv[1], "d") && estr_eq(argv[2], "e") && estr_eq(argv[3], "f"));
    cu_list_free(argv, argc);
    assert(cmder_args("test a \"b c\"    \"\"   d \"\"", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    assert(cmder_args("test a \"b c\"    \"\"   d \"x\"", &argc, &argv) == CU_OK);
    assert(argc == 5);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d") && estr_eq(argv[4], "x"));
    cu_list_free(argv, argc);
    assert(cmder_args("\"\"   test a \"b c\"    \"\"   \"d\"  ", &argc, &argv) == CU_OK);
    assert(argc == 4);
    assert(argv && estr_eq(argv[0], "test") && estr_eq(argv[1], "a") && 
        estr_eq(argv[2], "b c") && estr_eq(argv[3], "d"));
    cu_list_free(argv, argc);
    assert(cmder_args("a \"b \\\"c\\\" d\"", &argc, &argv) == CU_OK); // a "b \"c\" d"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "b \"c\" d"));
    cu_list_free(argv, argc);
    assert(cmder_args("a \\\"c\\\"", &argc, &argv) == CU_OK); // a \"c\"
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "a") && estr_eq(argv[1], "\"c\""));
    cu_list_free(argv, argc);
    assert(cmder_args("\\\"a\\\" b", &argc, &argv) == CU_OK); // \"a\" b
    assert(argc == 2);
    assert(argv && estr_eq(argv[0], "\"a\"") && estr_eq(argv[1], "b"));
    cu_list_free(argv, argc);
}

static int _argc;
static char** _argv;

void test_args_ptrs_cb(cmder_cmdval_t* cmdval) {
    if(cmdval->error != CMDER_CMDVAL_NO_ERROR) {
        return;
    }

    cmder_optval_t* s = NULL;
    assert(cmder_get_optval(cmdval, 's', &s) == CU_OK);
    assert(s);
    assert(estr_eq(s->val, "hello world"));

    // it must be the pointer to the same address
    // which means that value is not copied but referenced
    assert(s->val == _argv[2]);
}

static void test_args_ptrs() {
    assert(cmder_args("test -s \"hello world\"", &_argc, &_argv) == CU_OK);
    assert(_argv);
    assert(_argc == 3);
    assert(estr_eq(_argv[0], "test"));
    assert(estr_eq(_argv[1], "-s"));
    assert(estr_eq(_argv[2], "hello world"));

    cmder_handle_t cmder = NULL;
    assert(cmder_create(&(cmder_t){ .name = "cmder" }, &cmder) == CU_OK);
    assert(cmder);
    cmder_cmd_handle_t test;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "test", .callback = &test_args_ptrs_cb }, &test) == CU_OK);
    assert(cmder_add_vopt(test, &(cmder_opt_t) { .name = 's', .is_arg = true, .is_optional = false }) == CU_OK);
    assert(cmder_vrun_args(cmder, _argc, _argv) == CU_OK);

    cu_list_free(_argv, _argc);
    assert(cmder_destroy(cmder) == CU_OK);
}

// Testing error callback handling

static cmder_cmdval_err_t cmdval_err;
static bool error_triggered = false;
static bool error_cb_error = false;
static char unknown_option;
static bool capture_extra0 = false;
static bool capture_extra1 = false;
static bool capture_extra2 = false;
static char* extra0;
static char* extra1;
static char* extra2;
static bool capture_b_val;
static char* bval = NULL;
static bool xflag;
static bool yflag;
static bool zflag;

static void error_cb(cmder_cmdval_t* cmdval) {
    error_triggered = true;
    cmdval_err = cmdval->error;
    unknown_option = '\0';

    if(capture_extra0 && cmdval->extra_args_len > 0) {
        extra0 = strdup(cmdval->extra_args[0]);
    }

    if(capture_extra1 && cmdval->extra_args_len > 1) {
        extra1 = strdup(cmdval->extra_args[1]);
    }

    if(capture_extra2 && cmdval->extra_args_len > 2) {
        extra2 = strdup(cmdval->extra_args[2]);
    }

    if(cmdval_err != CMDER_CMDVAL_NO_ERROR) {
        error_cb_error = true;

        if(cmdval_err == CMDER_CMDVAL_UNKNOWN_OPTION) {
            unknown_option = cmdval->error_option_name;
        }
        
        char* errstr = NULL;
        size_t errstr_len = 0;
        assert(cmder_cmdval_errstr(cmdval, &errstr, &errstr_len) == CU_OK);
        assert(errstr);
        assert(errstr_len > 0);
        assert(strlen(errstr) == errstr_len);
        //printf("%s\n", errstr);
        free(errstr);

        return;
    }

    xflag = yflag = zflag = false;

    cmder_optval_t* x = NULL;
    cmder_optval_t* y = NULL;
    cmder_optval_t* z = NULL;

    cmder_get_optval(cmdval, 'x', &x);
    cmder_get_optval(cmdval, 'y', &y);
    cmder_get_optval(cmdval, 'z', &z);

    xflag = x && x->state;
    yflag = y && y->state;
    zflag = z && z->state;

    if(capture_b_val) {
        cmder_optval_t* b = NULL;
        if(cmder_get_optval(cmdval, 'b', &b) == CU_OK && b) {
            if(b->val) {
                bval = strdup(b->val);
            }
        }
    }

    error_cb_error = false;
    capture_extra0 =
    capture_extra1 =
    capture_extra2 = false;
    capture_b_val = false;
}

static void test_error_callback() {
    cmder_handle_t cmder = NULL;
    assert(cmder_create(&(cmder_t){ .name = "pc", .name_as_cmdline_prefix = true }, &cmder) == CU_OK);
    assert(cmder);
    cmder_cmd_handle_t error = NULL;
    assert(cmder_add_cmd(cmder, &(cmder_cmd_t){ .name = "error", .callback = &error_cb }, &error) == CU_OK);
    assert(error);
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error") == CU_OK); // it's ok to run without options
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_extra0 = capture_extra1 = capture_extra2 = true;
    assert(cmder_vrun(cmder, "pc error a b c") == CU_OK); // it's ok to run without options and with extra args
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(estr_eq(extra0, "a"));
    assert(estr_eq(extra1, "b"));
    assert(estr_eq(extra2, "c"));
    free(extra0);
    free(extra1);
    free(extra2);
    extra0 = extra1 = extra2 = NULL;
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -- a b c") == CU_OK); // it's ok to run without options and with extra args
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -a") == CU_ERR_CMDER_UNKNOWN_OPTION);
    assert(error_triggered && error_cb_error && cmdval_err == CMDER_CMDVAL_UNKNOWN_OPTION && unknown_option == 'a');
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -a -- b") == CU_ERR_CMDER_UNKNOWN_OPTION);
    assert(error_triggered && error_cb_error && cmdval_err == CMDER_CMDVAL_UNKNOWN_OPTION && unknown_option == 'a');
    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'a' }) == CU_OK);
    assert(cmder_vrun(cmder, "pc error") == CU_OK); // it's ok to run without options since "a" is a flag
    assert(cmder_vrun(cmder, "pc error a b c") == CU_OK);
    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -a") == CU_OK); // it's ok to run with "a" option now
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'b', .is_arg = true , .is_optional = true}) == CU_OK);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error") == CU_OK); // it's ok, a i b are optional
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_b_val = true;
    bval = NULL;
    assert(cmder_vrun(cmder, "pc error -b") == CU_OK); // it's ok, b is optional
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(!bval);
    free(bval);
    bval = NULL;

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_extra0 = capture_extra1 = true;
    assert(cmder_vrun(cmder, "pc error -- -a -b") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(estr_eq(extra0, "-a"));
    assert(estr_eq(extra1, "-b"));
    free(extra0);
    free(extra1);
    extra0 = extra1 = NULL;

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_b_val = true;
    bval = NULL;
    assert(cmder_vrun(cmder, "pc error -b \"ok bro\"") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(bval && estr_eq(bval, "ok bro"));
    free(bval);
    bval = NULL;

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_b_val = true;
    bval = NULL;
    assert(cmder_vrun(cmder, "pc error - - - -b \"ok bro\"") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(bval && estr_eq(bval, "ok bro"));
    free(bval);
    bval = NULL;

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_b_val = true;
    bval = NULL;
    capture_extra0 = true;
    assert(cmder_vrun(cmder, "pc error -b \"ok bro\" ext") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(estr_eq(bval, "ok bro"));
    assert(estr_eq(extra0, "ext"));
    free(bval);
    free(extra0);
    extra0 = NULL;
    bval = NULL;

    // flags test

    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'x' }) == CU_OK);
    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'y' }) == CU_OK);
    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'z' }) == CU_OK);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -x") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && xflag && !yflag && !zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -y") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && !xflag && yflag && !zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -z") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && !xflag && !yflag && zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -x -y -z") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && xflag && yflag && zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -xy -z") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && xflag && yflag && zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -xz") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && xflag && !yflag && zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -xyz") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR && xflag && yflag && zflag);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -xoz") == CU_ERR_CMDER_UNKNOWN_OPTION); // unknown opt "o"
    assert(error_triggered && error_cb_error && cmdval_err == CMDER_CMDVAL_UNKNOWN_OPTION);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_extra0 = true;
    assert(cmder_vrun(cmder, "pc error hey -x") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(estr_eq(extra0, "hey"));
    free(extra0);
    extra0 = NULL;

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    capture_extra0 = true;
    assert(cmder_vrun(cmder, "pc error \\\"") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
    assert(estr_eq(extra0, "\""));
    free(extra0);
    extra0 = NULL;

    // mandatory arg missing test

    assert(cmder_add_vopt(error, &(cmder_opt_t){ .name = 'u', .is_arg = true }) == CU_OK);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error") == CU_ERR_CMDER_OPT_VAL_MISSING);
    assert(error_triggered && error_cb_error && cmdval_err == CMDER_CMDVAL_OPTION_VALUE_MISSING);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -u") == CU_ERR_CMDER_OPT_VAL_MISSING);
    assert(error_triggered && error_cb_error && cmdval_err == CMDER_CMDVAL_OPTION_VALUE_MISSING);

    cmdval_err = CMDER_CMDVAL_NO_ERROR;
    error_triggered = error_cb_error = false;
    assert(cmder_vrun(cmder, "pc error -u xx") == CU_OK);
    assert(error_triggered && !error_cb_error && cmdval_err == CMDER_CMDVAL_NO_ERROR);
}