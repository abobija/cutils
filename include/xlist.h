#ifndef _CUTILS_XLIST_H_
#define _CUTILS_XLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct xnode* xnode_t;
typedef struct xlist* xlist_t;
typedef void(*xnode_free_fnc_t)(void* data);

struct xnode {
    xnode_t prev;
    xnode_t next;
    void* data;
};

struct xlist {
    xnode_t head;
    xnode_t tail;
    uint len;
    xnode_free_fnc_t data_free_fnc;
};

typedef struct {
    xnode_free_fnc_t data_free_fnc;
} xlist_config_t;

#define xlist_veach(list, CODE) \
    __extension__ ({ if(list) { xnode_t xnode = list->head; while(xnode) { \
        {CODE}; xnode = xnode->next; \
    } } list; })

#define xlist_each(data_type, list, CODE) \
    __extension__ ({ if(list) { xnode_t xnode = list->head; while(xnode) { \
        data_type xdata = (data_type) xnode->data; {CODE}; xnode = xnode->next; \
    } } list; })

#define xlist_get_tdata(list, index, data_type, ref) \
    __extension__ ({ \
        void* data = NULL; cu_err_t err = xlist_get_data(list, index, &data); \
        if(err == CU_OK) { ref = (data_type) data; } err; \
    })

#define xlist_vadd_to_back(list, data) xlist_add_to_back(list, data, NULL)
#define xlist_vadd_to_front(list, data) xlist_add_to_front(list, data, NULL)
#define xlist_add(list, data, node) xlist_add_to_back(list, data, node)
#define xlist_vadd(list, data)  xlist_add(list, data, NULL)

cu_err_t xlist_create(xlist_config_t* config, xlist_t* list);
int xlist_size(xlist_t list);
cu_err_t xlist_add_to_back(xlist_t list, void* data, xnode_t* node);
cu_err_t xlist_add_to_front(xlist_t list, void* data, xnode_t* node);
cu_err_t xlist_get(xlist_t list, int index, xnode_t* node);
cu_err_t xlist_get_data(xlist_t list, int index, void** data);
cu_err_t xlist_remove(xlist_t list, xnode_t node);

/**
 * @brief Remove all occurrences of data in list
 * @param list List
 * @param data Pointer to data
 * @return Number of deleted items on success, otherwise:
 *         CU_ERR_INVALID_ARG;
 *         CU_ERR_NOT_FOUND
 */
int xlist_remove_data(xlist_t list, void* data);
bool xlist_is_empty(xlist_t list);

/**
 * @brief Remove all items from list
 * @param list List
 * @return Number of deleted items on success, otherwise:
 *         CU_ERR_INVALID_ARG
 */
int xlist_flush(xlist_t list);
cu_err_t xlist_destroy(xlist_t list);

#ifdef __cplusplus
}
#endif

#endif