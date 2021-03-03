#ifndef _CUTILS_H_
#define _CUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int cu_err_t;

#define CU_OK 0
#define CU_FAIL -1
#define CU_ERR_INVALID_ARG -2
#define CU_ERR_NO_MEM -3

/**
 * @brief Universal struct (type) constructor. User is responsible for freeing the resulting object
 * @param type Struct type name (ex: person_t)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define cu_ctor(type, ...) ({ type* obj = calloc(1, sizeof(type)); if(obj) { *obj = (type){ __VA_ARGS__ }; } obj; })

/**
 * @brief Universal handle constructor. User is responsible for freeing the resulting object
 * @param handle_type Handle type (ex: person_handle_t)
 * @param struct Struct (ex: struct person)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define cu_ctor2(handle_type, struct, ...) ({ handle_type obj = calloc(1, sizeof(struct)); if(obj) { *obj = (struct){ __VA_ARGS__ }; } obj; })

/**
 * @brief Free the list (array of pointers)
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @param item_free_fnc Function for freeing the list item
 * @return void
 */
#define cu_list_free_(list, len, item_free_fnc) \
    ({ for(size_t i = 0; i < len; i++) { item_free_fnc(list[i]); list[i] = NULL; } free(list); list = NULL; len = 0; })

/**
 * @brief Free the list (array of pointers). For the freeing the list items free() will be used.
 *        If you want to use custom function for freeing list items, please use cu_list_free_ instead.
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @return void
 */
#define cu_list_free(list, len) cu_list_free_(list, len, free)

#ifdef __cplusplus
}
#endif

#endif