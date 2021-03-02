#include "../custring.h"
#include "../cutils.h"
#include <assert.h>

static void test_custreq() {
    assert( custreq("a", "a"));
    assert( custreq(" ", " "));
    assert( custreq("", ""));
    assert(!custreq(" ", ""));
    assert(!custreq(NULL, "b"));
    assert(!custreq("a", NULL));
    assert(!custreq(NULL, NULL));
}

static void test_custrneq() {
    assert( custrneq("a", "a", 1));
    assert( custrneq("a", "ab", 1));
    assert( custrneq(" ", " ",1));
    assert( custrneq("", "", 0));
    assert( custrneq("", "", 1));
    assert(!custrneq(" ", "", 1));
    assert(!custrneq(NULL, "b", 1));
    assert(!custrneq("a", NULL, 1));
    assert(!custrneq(NULL, NULL, 0));
}

static void test_custrsw() {
    assert( custrsw("+react", "+react"));
	assert( custrsw("+react to this", "+react"));
	assert(!custrsw("+react", "+react to this"));
	assert(!custrsw(NULL, "+react"));
	assert(!custrsw("+react", NULL));
	assert(!custrsw(NULL, NULL));
	assert(!custrsw("", "+react"));
	assert(!custrsw("+react", ""));
	assert(!custrsw("", ""));
	assert(!custrsw("+react", "   "));
	assert(!custrsw("  ", "+react"));
}

static void test_custrew() {
    assert( custrew("+react", "+react"));
	assert( custrew("+react", "ct"));
	assert( custrew("a", "a"));
	assert( custrew("abcd", "cd"));
	assert(!custrew("+react", "this to +react"));
	assert(!custrew(NULL, "+react"));
	assert(!custrew("+react", NULL));
	assert(!custrew(NULL, NULL));
	assert(!custrew("", "+react"));
	assert(!custrew("+react", ""));
	assert(!custrew("", ""));
	assert(!custrew("+react", "   "));
	assert(!custrew("  ", "+react"));
}

static void test_custrn_is_digit_only() {
    assert( custrn_is_digit_only("123", 3));
    assert( custrn_is_digit_only("", 0));
    assert( custrn_is_digit_only("1", 1));
    assert( custrn_is_digit_only("123", 2));
    assert( custrn_is_digit_only("123", 5));
    assert(!custrn_is_digit_only("a", 1));
    assert( custrn_is_digit_only("23ab", 2));
    assert(!custrn_is_digit_only("abcs", 2));
    assert( custrn_is_digit_only("", 3));
}

static void test_custrn_chrcnt() {
    assert(custrn_chrcnt(".34.f.s.", '.', 8) == 4);
    assert(custrn_chrcnt(".34.f.s.", '.', 3) == 1);
    assert(custrn_chrcnt(".34.f.s.", '.', 5) == 2);
}

static void test_custrsplit() {
    char** _pcs;
    size_t _len;

    _pcs = custrsplit("a,b,,c", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 1 && strlen(_pcs[2]) == 1 && 
        custreq(_pcs[0], "a") && custreq(_pcs[1], "b") && custreq(_pcs[2], "c")
    );
    cufree_list(_pcs, _len);

    _pcs = custrsplit(",,d", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && custreq(_pcs[0], "d"));
    cufree_list(_pcs, _len);

    _pcs = custrsplit("a,,bc,d,,", ',', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        custreq(_pcs[0], "a") && custreq(_pcs[1], "bc") && custreq(_pcs[2], "d")
    );
    cufree_list(_pcs, _len);

    _pcs = custrsplit("da", ',', &_len);
    assert(_len == 1 && custreq(_pcs[0], "da"));
    cufree_list(_pcs, _len);

    _pcs = custrsplit("d,", ',', &_len);
    assert(_len == 1);
    assert(strlen(_pcs[0]) == 1 && custreq(_pcs[0], "d"));
    cufree_list(_pcs, _len);

    _pcs = custrsplit("jan.feb.mar", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        custreq(_pcs[0], "jan") && custreq(_pcs[1], "feb") && custreq(_pcs[2], "mar")
    );
    cufree_list(_pcs, _len);

    _pcs = custrsplit("..jan..feb..mar..", '.', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 3 && strlen(_pcs[1]) == 3 && strlen(_pcs[2]) == 3 && 
        custreq(_pcs[0], "jan") && custreq(_pcs[1], "feb") && custreq(_pcs[2], "mar")
    );
    cufree_list(_pcs, _len);

    _pcs = custrsplit("......", '.', &_len);
    assert(_len == 0 && !_pcs);
    cufree_list(_pcs, _len);

    _pcs = custrsplit("     a     bc  d    ", ' ', &_len);
    assert(_len == 3);
    assert(strlen(_pcs[0]) == 1 && strlen(_pcs[1]) == 2 && strlen(_pcs[2]) == 1 && 
        custreq(_pcs[0], "a") && custreq(_pcs[1], "bc") && custreq(_pcs[2], "d")
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