#ifndef _CUTILS_H_
#define _CUTILS_H_

/**
 * @brief Universal struct (type) constructor. User is responsible for freeing the resulting object
 * @param type Struct type name (ex: discord_person_t)
 * @param ... Struct attributes (ex: .id = 2, .name = strdup("John"))
 * @return Pointer to dynamically allocated struct
 */
#define cuctor(type, ...) ({ type* obj = calloc(1, sizeof(type)); if(obj) { *obj = (type){ __VA_ARGS__ }; } obj; })

/**
 * @brief Free the list (array of pointers)
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @param item_free_fnc Function for freeing the list item
 * @return void
 */
#define cufree_list_(list, len, item_free_fnc) \
    ({ for(size_t i = 0; i < len; i++) { item_free_fnc(list[i]); } free(list); })

/**
 * @brief Free the list (array of pointers). For the freeing the list items free() will be used.
 *        If you want to use custom function for freeing list items, please use cufree_list_ instead.
 * @param list Double pointer to list
 * @param len Number of the items in the list
 * @return void
 */
#define cufree_list(list, len) cufree_list_(list, len, free)

#endif