#include "../estr.h"
#include "../cutils.h"
#include <assert.h>

static void test_estr_eq() {
    assert( estr_eq("a", "a"));
    assert( estr_eq(" ", " "));
    assert( estr_eq("", ""));
    assert(!estr_eq(" ", ""));
    assert(!estr_eq(NULL, "b"));
    assert(!estr_eq("a", NULL));
    assert(!estr_eq(NULL, NULL));
}

static void test_estrn_eq() {
    assert( estrn_eq("a", "a", 1));
    assert( estrn_eq("a", "ab", 1));
    assert( estrn_eq(" ", " ",1));
    assert( estrn_eq("", "", 0));
    assert( estrn_eq("", "", 1));
    assert(!estrn_eq(" ", "", 1));
    assert(!estrn_eq(NULL, "b", 1));
    assert(!estrn_eq("a", NULL, 1));
    assert(!estrn_eq(NULL, NULL, 0));
}

static void test_estr_sw() {
    assert( estr_sw("+react", "+react"));
    assert( estr_sw("+react to this", "+react"));
    assert(!estr_sw("+react", "+react to this"));
    assert(!estr_sw(NULL, "+react"));
    assert(!estr_sw("+react", NULL));
    assert(!estr_sw(NULL, NULL));
    assert(!estr_sw("", "+react"));
    assert(!estr_sw("+react", ""));
    assert(!estr_sw("", ""));
    assert(!estr_sw("+react", "   "));
    assert(!estr_sw("  ", "+react"));
}

static void test_estr_ew() {
    assert( estr_ew("+react", "+react"));
    assert( estr_ew("+react", "ct"));
    assert( estr_ew("a", "a"));
    assert( estr_ew("abcd", "cd"));
    assert(!estr_ew("+react", "this to +react"));
    assert(!estr_ew(NULL, "+react"));
    assert(!estr_ew("+react", NULL));
    assert(!estr_ew(NULL, NULL));
    assert(!estr_ew("", "+react"));
    assert(!estr_ew("+react", ""));
    assert(!estr_ew("", ""));
    assert(!estr_ew("+react", "   "));
    assert(!estr_ew("  ", "+react"));
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

static void test_estr_split() {
    char** _pcs;
    size_t _len;

    _pcs = estr_split("a,b,,c", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 1 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "b") && estr_eq(_pcs[2], "c")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split(",,d", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estr_eq(_pcs[0], "d"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("a,,bc,d,,", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "bc") && estr_eq(_pcs[2], "d")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("da", ',', &_len);
    assert(_len == 1 && estr_eq(_pcs[0], "da"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("d,", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estr_eq(_pcs[0], "d"));
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("jan.feb.mar", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estr_eq(_pcs[0], "jan") && estr_eq(_pcs[1], "feb") && estr_eq(_pcs[2], "mar")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("..jan..feb..mar..", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estr_eq(_pcs[0], "jan") && estr_eq(_pcs[1], "feb") && estr_eq(_pcs[2], "mar")
    );
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("......", '.', &_len);
    assert(_len == 0 && !_pcs);
    cu_list_tfree(_pcs, size_t, _len);

    _pcs = estr_split("     a     bc  d    ", ' ', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estr_eq(_pcs[0], "a") && estr_eq(_pcs[1], "bc") && estr_eq(_pcs[2], "d")
    );
    cu_list_tfree(_pcs, size_t, _len);
}

static void test_estr_cat() {
    char* res;
    
    res = estr_cat("a", "b", "c");
    assert(estr_eq(res, "abc"));
    free(res);

    res = estr_cat("", "b", " ");
    assert(estr_eq(res, "b "));
    free(res);

    res = estr_cat("a", "b", NULL, "c"); // c will be ignored
    assert(estr_eq(res, "ab"));
    free(res);
}

static void test_estr_url_encode() {
    char* res;

    res = estr_url_encode("👍");
    assert(estr_eq(res, "%f0%9f%91%8d"));
    free(res);
}

static void test_estr_rep() {
    char* tmp;
    tmp = estr_rep("aaabbbccc", "bbb", "ddd");
    assert(estr_eq(tmp, "aaadddccc"));
    free(tmp);
    tmp = estr_rep("aaa\\\"x\\\"d", "\\\"", "\""); // aaa\"x\"d -> aaa"x"d
    assert(estr_eq(tmp, "aaa\"x\"d"));
    free(tmp);
    tmp = estr_rep("abc", "d", "x");
    assert(estr_eq(tmp, "abc"));
    free(tmp);
    tmp = estr_rep(NULL, "b", "x");
    assert(!tmp);
    tmp = estr_rep("abc", NULL, "x");
    assert(!tmp);
    tmp = estr_rep("abc", "b", NULL);
    assert(!tmp);
}

int main() {
    test_estr_eq();
    test_estrn_eq();
    test_estr_sw();
    test_estr_ew();
    test_estrn_is_digit_only();
    test_estrn_chrcnt();
    test_estr_split();
    test_estr_cat();
    test_estr_url_encode();
    test_estr_rep();

    return 0;
}