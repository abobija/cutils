#include "xlist.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char* name;
} person_t;

static person_t* john = NULL;
static person_t* marry = NULL;

static void free_data(void* data) {
    person_t* person = (person_t*) data;

    if(!person) {
        return;
    }

    free(person->name);
    free(person);

    if(person == john)       { john = NULL; }
    else if(person == marry) { marry = NULL; }
}

static void test_dynmem() {
    xlist_t list = NULL;
    assert(xlist_create(&(xlist_config_t) {
        .data_free_fnc = &free_data
    }, &list) == CU_OK);
    assert(xlist_is_empty(list));
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == CU_OK);
    assert(!xlist_is_empty(list));
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == CU_OK);
    assert(!xlist_is_empty(list));
    assert(xlist_size(list) == 2);
    assert(xlist_remove_data(list, john) == 1);
    assert(!john);
    assert(marry);
    assert(xlist_size(list) == 1);
    assert(xlist_remove_data(list, marry) == 1);
    assert(!john);
    assert(!marry);
    assert(xlist_size(list) == 0);
    assert(xlist_flush(list) == 0);
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == CU_OK);
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == CU_OK);
    assert(john && marry);
    assert(xlist_flush(list) == 2); // free both persons
    assert(!john && !marry);
    assert(xlist_destroy(list) == CU_OK); // vdestroy because it's already flushed

    list = NULL;
    assert(xlist_create(&(xlist_config_t) {
        .data_free_fnc = &free_data
    }, &list) == CU_OK);
    assert(xlist_vadd(list, john = cu_ctor(person_t, .name = strdup("John"))) == CU_OK);
    assert(xlist_vadd(list, marry = cu_ctor(person_t, .name = strdup("Marry"))) == CU_OK);
    assert(john && marry);
    assert(xlist_destroy(list) == CU_OK); // custom free fnc
    assert(!john && !marry);
}

int main() {
    test_dynmem();

    xlist_t list = NULL;
    assert(xlist_size(list) == CU_FAIL); // list is null

    assert(xlist_create(NULL, &list) == CU_OK);
    assert(xlist_size(list) == 0);

    int a = 5;
    int b = 4;
    int c = 6;
    void* data = NULL;

    assert(xlist_vadd(list, &a) == CU_OK);
    assert(xlist_size(list) == 1);
    assert(xlist_get_data(list, 1, &data) == CU_ERR_NOT_FOUND);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_remove_data(list, data) == 1); // remove a
    assert(xlist_size(list) == 0);
    assert(xlist_get_data(list, 0, &data) == CU_ERR_NOT_FOUND);

    assert(xlist_vadd(list, &a) == CU_OK);
    assert(xlist_vadd(list, &a) == CU_OK); // add a again
    assert(xlist_size(list) == 2);
    assert(xlist_get_data(list, 1, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_remove_data(list, data) == 2); // delete both occurrences
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == CU_OK);
    assert(xlist_vadd(list, &b) == CU_OK);
    assert(xlist_size(list) == 2);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_get_data(list, 1, &data) == CU_OK);
    assert(*(int*) data == b);
    assert(xlist_remove_data(list, data) == 1); // remove b
    assert(xlist_size(list) == 1);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == a);
    assert(xlist_get_data(list, 1, &data) == CU_ERR_NOT_FOUND);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(xlist_remove_data(list, data) == 1); // remove a
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == CU_OK);
    assert(xlist_vadd(list, &b) == CU_OK);
    assert(xlist_vadd(list, &c) == CU_OK);
    assert(xlist_size(list) == 3);
    assert(xlist_flush(list) == 3); // remove all
    assert(xlist_size(list) == 0);

    assert(xlist_vadd(list, &a) == CU_OK);
    assert(xlist_vadd_to_front(list, &b) == CU_OK);
    assert(xlist_get_data(list, 0, &data) == CU_OK);
    assert(*(int*) data == b); // b should be in the front

    // test references

    xnode_t node = NULL;
    xnode_t node_tmp = NULL;
    int x = 10;
    assert(xlist_add_to_front(list, &x, &node) == CU_OK);
    assert(xlist_get(list, 0, &node_tmp) == CU_OK);
    assert(node == node_tmp);

    assert(xlist_destroy(list) == CU_OK);

    return 0;
}