#include "../estr.h"
#include "../cutils.h"
#include <assert.h>

static void test_custreq() {
    assert( estreq("a", "a"));
    assert( estreq(" ", " "));
    assert( estreq("", ""));
    assert(!estreq(" ", ""));
    assert(!estreq(NULL, "b"));
    assert(!estreq("a", NULL));
    assert(!estreq(NULL, NULL));
}

static void test_custrneq() {
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

static void test_custrsw() {
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

static void test_custrn_is_digit_only() {
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

static void test_custrn_chrcnt() {
    assert(estrn_chrcnt(".34.f.s.", '.', 8) == 4);
    assert(estrn_chrcnt(".34.f.s.", '.', 3) == 1);
    assert(estrn_chrcnt(".34.f.s.", '.', 5) == 2);
}

static void test_custrsplit() {
    char** _pcs;
    size_t _len;

    _pcs = estrsplit("a,b,,c", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 1 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "b") && estreq(_pcs[2], "c")
    );
    cufree_list(_pcs, _len);

    _pcs = estrsplit(",,d", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estreq(_pcs[0], "d"));
    cufree_list(_pcs, _len);

    _pcs = estrsplit("a,,bc,d,,", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "bc") && estreq(_pcs[2], "d")
    );
    cufree_list(_pcs, _len);

    _pcs = estrsplit("da", ',', &_len);
    assert(_len == 1 && estreq(_pcs[0], "da"));
    cufree_list(_pcs, _len);

    _pcs = estrsplit("d,", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && estreq(_pcs[0], "d"));
    cufree_list(_pcs, _len);

    _pcs = estrsplit("jan.feb.mar", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estreq(_pcs[0], "jan") && estreq(_pcs[1], "feb") && estreq(_pcs[2], "mar")
    );
    cufree_list(_pcs, _len);

    _pcs = estrsplit("..jan..feb..mar..", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        estreq(_pcs[0], "jan") && estreq(_pcs[1], "feb") && estreq(_pcs[2], "mar")
    );
    cufree_list(_pcs, _len);

    _pcs = estrsplit("......", '.', &_len);
    assert(_len == 0 && !_pcs);
    cufree_list(_pcs, _len);

    _pcs = estrsplit("     a     bc  d    ", ' ', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        estreq(_pcs[0], "a") && estreq(_pcs[1], "bc") && estreq(_pcs[2], "d")
    );
    cufree_list(_pcs, _len);
}

int main() {
    test_custreq();
    test_custrneq();
    test_custrsw();
    test_custrn_is_digit_only();
    test_custrn_chrcnt();
    test_custrsplit();

    return 0;
}