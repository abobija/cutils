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

void echo_cb(cmder_cmd_val_t* cmdval) {
    echo_fired = true;
    assert(cmdval->cmder);
    assert(cmdval->opts);
    assert(cmdval->opts_len == 1);
    cmder_opt_val_t* optval;
    assert(optval = cmder_opt_val('m', cmdval));
    assert(optval->val);
    assert(!optval->state);
    assert(optval->opt->is_arg);
    assert(!optval->opt->is_optional);
    assert(optval->opt->name == 'm');
    echo_message = strdup(optval->val);
    assert(! cmder_opt_val('x', cmdval));
}

void ok_cb(cmder_cmd_val_t* cmdval) {
    ok_fired = true;
    assert(cmdval->cmd);
    assert(cmdval->cmd == ok);
}

void null_cb(cmder_cmd_val_t* cmdval) { /* noop */ }

static void test_getoopts(cmder_handle_t cmder);
static void test_argv();

int main() {
    test_argv();

    cmder_handle_t cmder = cmder_create(&(cmder_t){
        .name = "+esp32",
        .context = &data
    });

    assert(cmder);

    test_getoopts(cmder);

    assert(cmder_cmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    }));

    cmder_cmd_handle_t echo = cmder_cmd(cmder, &(cmder_cmd_t){
        .name = "echo",
        .callback = &echo_cb
    });

    assert(echo);

    assert(cmder_opt(echo, &(cmder_opt_t){
        .name = 'm',
        .is_arg = true
    }) == 0);

    assert(ok = cmder_cmd(cmder, &(cmder_cmd_t){ 
        .name = "ok",
        .callback = &ok_cb
    }));

    assert(cmder_opt(echo, &(cmder_opt_t){ .name = 'm' }) != 0); // already exist
    assert(! cmder_cmd(cmder, &(cmder_cmd_t){ .name = "kkk" })); // no callback provided

    assert(cmder_run(cmder, "+") != 0); // wrong cmder name
    assert(cmder_run(cmder, "esp32") != 0); // wrong cmder name
    assert(cmder_run(cmder, "+esp32") != 0); // no cmd name
    assert(cmder_run(cmder, "+esp32 test") != 0); // test cmd does not exists
    assert(cmder_run(cmder, "+esp32 echo") != 0); // missing mandatory option m
    assert(!echo_fired);
    assert(cmder_run(cmder, "+esp32 echo -x") != 0); // unknown option
    assert(!echo_fired);
    assert(cmder_run(cmder, "+esp32 echo -m") != 0); // m is arg and argval missing
    assert(!echo_fired);
    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m hey") == 0); // ok
    assert(echo_fired);
    echo_fired = false;
    assert(estreq(echo_message, "hey"));
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m whats up") == 0); // ok, "up" extra arg
    assert(echo_fired);
    assert(estreq(echo_message, "whats"));
    free(echo_message);
    echo_message = NULL;
    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -- -m hey") != 0); // parsing terminated with --, missing m
    assert(!echo_fired);
    help_fired = false;
    assert(cmder_run(cmder, "+esp32 help") == 0); // ok
    assert(help_fired);
    ok_fired = false;
    assert(cmder_run(cmder, "+esp32 ok") == 0); // ok
    assert(ok_fired);

    const char* too_long_cmd = "+esp32 echo -m aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaa";
    
    assert(cmder_run(cmder, too_long_cmd) != 0); // default max is 256, this command is 274

    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m \"whats up\"") == 0); // ok
    assert(echo_fired);
    assert(estreq(echo_message, "whats up"));
    free(echo_message);
    echo_message = NULL;

    echo_fired = false;
    assert(cmder_run(cmder, "+esp32 echo -m \"this is \\\"quoted\\\" word\"") == 0); // ok
    assert(echo_fired);
    assert(estreq(echo_message, "this is \"quoted\" word"));
    free(echo_message);
    echo_message = NULL;

    cmder_destroy(cmder);

    return 0;
}


static void test_getoopts(cmder_handle_t cmder) {
    char* tmp;

    cmder_cmd_handle_t cmplx = cmder_cmd(cmder, &(cmder_cmd_t){ .name = "c1", .callback = &null_cb });
    assert(cmplx);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'a' }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'b', .is_arg = true }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'c', .is_arg = true, .is_optional = true }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'd' }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'e', .is_arg = true, .is_optional = true }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'f', .is_arg = true }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'g' }) == 0);
    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'h' }) == 0);
    assert(tmp = cmder_getoopts(cmplx));
    assert(estreq(tmp, ":ab:c::de::f:gh"));
    free(tmp);
}

static void test_argv() {
    int argc;
    char** argv;
    argv = cmder_argv("test a b c", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "a") && 
        estreq(argv[2], "b") && estreq(argv[3], "c"));
    culist_free(argv, argc);
    argv = cmder_argv("test ab \"c d\" e", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "ab") &&
        estreq(argv[2], "c d") && estreq(argv[3], "e"));
    culist_free(argv, argc);
    argv = cmder_argv("test a \"b c\"    \"\"   d", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "a") && 
        estreq(argv[2], "b c") && estreq(argv[3], "d"));
    culist_free(argv, argc);
    argv = cmder_argv("  \" a   b c \"  d e \"f\" ", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], " a   b c ") && 
        estreq(argv[1], "d") && estreq(argv[2], "e") && estreq(argv[3], "f"));
    culist_free(argv, argc);
    argv = cmder_argv("ab \"", &argc);
    assert(!argv && argc == 0); // syntax error
    argv = cmder_argv("test a \"b c\"    \"\"   d \"\"", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "a") && 
        estreq(argv[2], "b c") && estreq(argv[3], "d"));
    culist_free(argv, argc);
    argv = cmder_argv("test a \"b c\"    \"\"   d \"x\"", &argc);
    assert(argc == 5);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "a") && 
        estreq(argv[2], "b c") && estreq(argv[3], "d") && estreq(argv[4], "x"));
    culist_free(argv, argc);
    argv = cmder_argv("\"\"   test a \"b c\"    \"\"   \"d\"  ", &argc);
    assert(argc == 4);
    assert(argv && estreq(argv[0], "test") && estreq(argv[1], "a") && 
        estreq(argv[2], "b c") && estreq(argv[3], "d"));
    culist_free(argv, argc);
    argv = cmder_argv("a \"b \\\"c\\\" d\"", &argc); // a "b \"c\" d"
    assert(argc == 2);
    assert(argv && estreq(argv[0], "a") && estreq(argv[1], "b \"c\" d"));
    culist_free(argv, argc);
    argv = cmder_argv("a \\\"c\\\"", &argc); // a \"c\"
    assert(argc == 2);
    assert(argv && estreq(argv[0], "a") && estreq(argv[1], "\"c\""));
    culist_free(argv, argc);
    argv = cmder_argv("\\\"a\\\" b", &argc); // \"a\" b
    assert(argc == 2);
    assert(argv && estreq(argv[0], "\"a\"") && estreq(argv[1], "b"));
    culist_free(argv, argc);
}