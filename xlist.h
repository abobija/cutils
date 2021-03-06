#ifndef _CUTILS_XLIST_H_
#define _CUTILS_XLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include <stdlib.h>

typedef struct xnode* xnode_t;
typedef struct xlist* xlist_t;
typedef void(*xnode_free_fnc_t)(void* data);

#define xlist_vadd_to_back(list, data) xlist_add_to_back(list, data, NULL)
#define xlist_vadd_to_front(list, data) xlist_add_to_front(list, data, NULL)
#define xlist_add(list, data, node) xlist_add_to_back(list, data, node)
#define xlist_vadd(list, data)  xlist_add(list, data, NULL)

#define xlist_vremove(list, node) xlist_remove(list, node, NULL)
#define xlist_vremove_data(list, data) xlist_remove_data(list, data, NULL)
#define xlist_vflush(list) xlist_flush(list, NULL)
#define xlist_vdestroy(list) xlist_destroy(list, NULL)

cu_err_t xlist_create(xlist_t* list);
int xlist_size(xlist_t list);
cu_err_t xlist_add_to_back(xlist_t list, void* data, xnode_t* node);
cu_err_t xlist_add_to_front(xlist_t list, void* data, xnode_t* node);
cu_err_t xlist_get(xlist_t list, int index, xnode_t* node);
cu_err_t xlist_get_data(xlist_t list, int index, void** data);
cu_err_t xlist_remove(xlist_t list, xnode_t node, xnode_free_fnc_t free_fnc);
cu_err_t xlist_remove_data(xlist_t list, void* data, xnode_free_fnc_t free_fnc);
cu_err_t xlist_flush(xlist_t list, xnode_free_fnc_t free_fnc);
cu_err_t xlist_destroy(xlist_t list, xnode_free_fnc_t free_fnc);

#ifdef __cplusplus
}
#endif

#endif