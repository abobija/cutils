#include "../estr.h"
#include "../cutils.h"
#include <assert.h>

static void test_estreq() {
    assert( estreq("a", "a"));
    assert( estreq(" ", " "));
    assert( estreq("", ""));
    assert(!estreq(" ", ""));
    assert(!estreq(NULL, "b"));
    assert(!estreq("a", NULL));
    assert(!estreq(NULL, NULL));
}

static void test_estrneq() {
    assert( estrneq("a", "a", 1));
    assert( estrneq("a", "ab", 1));
    assert( estrneq(" ", " ",1));
    assert( estrneq("", "", 0));
    assert( estrneq("", "", 1));
    assert(!estrneq(" ", "", 1));
    assert(!estrneq(NULL, "b", 1));
    assert(!estrneq("a", NULL, 1));
    assert(!estrneq(NULL, NULL, 0));
}

static void test_estrsw() {
    assert( estrsw("+react", "+react"));
    assert( estrsw("+react to this", "+react"));
    assert(!estrsw("+react", "+react to this"));
    assert(!estrsw(NULL, "+react"));
    assert(!estrsw("+react", NULL));
    assert(!estrsw(NULL, NULL));
    assert(!estrsw("", "+react"));
    assert(!estrsw("+react", ""));
    assert(!estrsw("", ""));
    assert(!estrsw("+react", "   "));
    assert(!estrsw("  ", "+react"));
}

static void test_custrew() {
    assert( estrew("+react", "+react"));
    assert( estrew("+react", "ct"));
    assert( estrew("a", "a"));
    assert( estrew("abcd", "cd"));
    assert(!estrew("+react", "this to +react"));
    assert(!estrew(NULL, "+react"));
    assert(!estrew("+react", NULL));
    assert(!estrew(NULL, NULL));
    assert(!estrew("", "+react"));
    assert(!estrew("+react", ""));
    assert(!estrew("", ""));
    assert(!estrew("+react", "   "));
    assert(!estrew("  ", "+react"));
}

static void test_estrn_is_digit_only() {
    assert( estrn_is_digit_only("123", 3));
    assert( estrn_is_digit_only("", 0));
    assert( estrn_is_digit_only("1", 1));
    assert( estrn_is_digit_only("123", 2));
    assert( estrn_is_digit_only("123", 5));
    assert(!estrn_is_digit_only("a", 1));
    assert( estrn_is_digit_only("23ab", 2));
    assert(!estrn_is_digit_only("abcs", 2));
    assert( estrn_is_digit_only("", 3));
}

static void test_estrn_chrcnt() {
    assert(estrn_chrcnt(".34.f.s.", '.', 8) == 4);
    assert(estrn_chrcnt(".34.f.s.", '.', 3) == 1);
    assert(estrn_chrcnt(".34.f.s.", '.', 5) == 2);
}

static void test_estrsplit() {
    char** _pcs;
    size_t _len;

    _pcs = estrsplit("a,b,,c", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 1 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "b") && estreq(_pcs[2], "c")
    );
    culist_free(_pcs, _len);

    _pcs = estrsplit(",,d", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estreq(_pcs[0], "d"));
    culist_free(_pcs, _len);

    _pcs = estrsplit("a,,bc,d,,", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "bc") && estreq(_pcs[2], "d")
    );
    culist_free(_pcs, _len);

    _pcs = estrsplit("da", ',', &_len);
    assert(_len == 1 && estreq(_pcs[0], "da"));
    culist_free(_pcs, _len);

    _pcs = estrsplit("d,", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estreq(_pcs[0], "d"));
    culist_free(_pcs, _len);

    _pcs = estrsplit("jan.feb.mar", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estreq(_pcs[0], "jan") && estreq(_pcs[1], "feb") && estreq(_pcs[2], "mar")
    );
    culist_free(_pcs, _len);

    _pcs = estrsplit("..jan..feb..mar..", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estreq(_pcs[0], "jan") && estreq(_pcs[1], "feb") && estreq(_pcs[2], "mar")
    );
    culist_free(_pcs, _len);

    _pcs = estrsplit("......", '.', &_len);
    assert(_len == 0 && !_pcs);
    culist_free(_pcs, _len);

    _pcs = estrsplit("     a     bc  d    ", ' ', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "bc") && estreq(_pcs[2], "d")
    );
    culist_free(_pcs, _len);
}

static void test_estrcat() {
    char* res;
    
    res = estrcat("a", "b", "c");
    assert(estreq(res, "abc"));
    free(res);

    res = estrcat("", "b", " ");
    assert(estreq(res, "b "));
    free(res);

    res = estrcat("a", "b", NULL, "c"); // c will be ignored
    assert(estreq(res, "ab"));
    free(res);
}

static void test_estr_url_encode() {
    char* res;

    res = estr_url_encode("👍");
    assert(estreq(res, "%f0%9f%91%8d"));
    free(res);
}

static void test_estrrep() {
    char* tmp;
    tmp = estrrep("aaabbbccc", "bbb", "ddd");
    assert(estreq(tmp, "aaadddccc"));
    free(tmp);
    tmp = estrrep("aaa\\\"x\\\"d", "\\\"", "\""); // aaa\"x\"d -> aaa"x"d
    assert(estreq(tmp, "aaa\"x\"d"));
    free(tmp);
    tmp = estrrep("abc", "d", "x");
    assert(estreq(tmp, "abc"));
    free(tmp);
    tmp = estrrep(NULL, "b", "x");
    assert(!tmp);
    tmp = estrrep("abc", NULL, "x");
    assert(!tmp);
    tmp = estrrep("abc", "b", NULL);
    assert(!tmp);
}

int main() {
    test_estreq();
    test_estrneq();
    test_estrsw();
    test_estrn_is_digit_only();
    test_estrn_chrcnt();
    test_estrsplit();
    test_estrcat();
    test_estr_url_encode();
    test_estrrep();

    return 0;
}