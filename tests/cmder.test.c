#include "../cmder.h"
#include "../estr.h"
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
}

void echo_cb(cmder_cmd_val_t* cmdval) {
    echo_fired = true;
    assert(cmdval->cmder);
    assert(cmdval->opts);
    assert(cmdval->opts_len == 1);
    echo_message = strdup(cmder_opt_val('m', cmdval)->val);
}

void ok_cb(cmder_cmd_val_t* cmdval) {
    ok_fired = true;
    assert(cmdval->cmd);
    assert(cmdval->cmd == ok);
}

void null_cb(cmder_cmd_val_t* cmdval) {
    // noop
}

int main() {
    char* tmp;

    cmder_handle_t cmder = cmder_create(&(cmder_t){
        .name = "+esp32",
        .context = &data
    });

    cmder_cmd(cmder, &(cmder_cmd_t){
        .name = "help",
        .callback = &help_cb
    });

    cmder_cmd_handle_t echo = cmder_cmd(cmder, &(cmder_cmd_t){
        .name = "echo",
        .callback = &echo_cb
    });

    cmder_opt(echo, &(cmder_opt_t){
        .name = 'm',
        .is_arg = true
    });

    ok = cmder_cmd(cmder, &(cmder_cmd_t){ 
        .name = "ok",
        .callback = &ok_cb
    });

    tmp = cmder_getoopts(echo);
    assert(estreq(tmp, ":m:"));
    free(tmp);

    cmder_cmd_handle_t cmplx = cmder_cmd(cmder, &(cmder_cmd_t){ .name = "c1", .callback = &null_cb });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'a' });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'b', .is_arg = true });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'c', .is_arg = true, .is_optional = true });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'd' });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'e', .is_arg = true, .is_optional = true });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'f', .is_arg = true });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'g' });
    cmder_opt(cmplx, &(cmder_opt_t){ .name = 'h' });
    tmp = cmder_getoopts(cmplx);
    assert(estreq(tmp, ":ab:c::de::f:gh"));
    free(tmp);

    assert(cmder_opt(cmplx, &(cmder_opt_t){ .name = 'h' }) != 0); // already exist

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

    cmder_destroy(cmder);

    return 0;
}