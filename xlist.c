#include "xlist.h"

struct xnode {
    void* data;
    xnode_t next;
};

struct xlist {
    xnode_t head;
    uint len;
};

cu_err_t xlist_create(xlist_t* list) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    cu_mem_checkr(*list = cu_tctor(xlist_t, struct xlist));

    return CU_OK;
}

int xlist_size(xlist_t list) {
    return !list ? CU_FAIL : (int) list->len;
}

cu_err_t xlist_add_to_back(xlist_t list, void* data, xnode_t* node) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    xnode_t _node = NULL;
    cu_mem_checkr(_node = cu_tctor(xnode_t, struct xnode, .data = data));
    list->len++;

    if(!list->head) {
        list->head = _node;
        return CU_OK;
    }

    xnode_t ptr = list->head;
    while(ptr->next) { ptr = ptr->next; }
    ptr->next = _node;

    if(node) { *node = _node; }

    return CU_OK;
}

cu_err_t xlist_add_to_front(xlist_t list, void* data, xnode_t* node) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    xnode_t _node = NULL;
    cu_mem_checkr(_node = cu_tctor(xnode_t, struct xnode, .data = data));
    list->len++;

    if(!list->head) {
        list->head = _node;
        return CU_OK;
    }

    _node->next = list->head;
    list->head = _node;

    if(node) { *node = _node; }

    return CU_OK;
}

cu_err_t xlist_get(xlist_t list, int index, xnode_t* node) {
    if(!list || !node || index < 0) {
        return CU_ERR_INVALID_ARG;
    }

    xnode_t ptr = list->head;
    int i = 0;

    while(ptr) {
        if(i++ == index) {
            *node = ptr;
            return CU_OK;
        }
        
        ptr = ptr->next;
    }

    return CU_ERR_NOT_FOUND;
}

cu_err_t xlist_get_data(xlist_t list, int index, void** data) {
    if(!list || !data || index < 0) {
        return CU_ERR_INVALID_ARG;
    }

    xnode_t ptr = list->head;
    int i = 0;

    while(ptr) {
        if(i++ == index) {
            *data = ptr->data;
            return CU_OK;
        }
        
        ptr = ptr->next;
    }

    return CU_ERR_NOT_FOUND;
}

cu_err_t xlist_remove(xlist_t list, xnode_t node, xnode_free_fnc_t free_fnc) {
    if(!list || !node) {
        return CU_ERR_INVALID_ARG;
    }

    if(list->head == NULL) {
        return CU_ERR_NOT_FOUND;
    }

    xnode_t prev = NULL;
    xnode_t curr = list->head;

    while(curr) {
        if(curr == node) {
            if(prev) { prev->next = curr->next; }
            else { list->head = curr->next; }
            curr->next = NULL;
            if(free_fnc) { free_fnc(curr->data); }
            free(curr);
            list->len--;
            return CU_OK;
        }

        prev = curr;
        curr = curr->next;
    }

    return CU_ERR_NOT_FOUND;
}

cu_err_t xlist_remove_data(xlist_t list, void* data, xnode_free_fnc_t free_fnc) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    xnode_t ptr = list->head;
    xnode_t next = NULL;

    while(ptr) {
        next = ptr->next;

        if(ptr->data == data) {
            cu_err_checkr(xlist_remove(list, ptr, free_fnc));
        }

        ptr = next;
    }

    return err;
}

cu_err_t xlist_flush(xlist_t list, xnode_free_fnc_t free_fnc) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;

    while(list->head) {
        cu_err_checkr(xlist_remove(list, list->head, free_fnc));
    }

    return err;
}

cu_err_t xlist_destroy(xlist_t list, xnode_free_fnc_t free_fnc) {
    if(!list) {
        return CU_ERR_INVALID_ARG;
    }

    cu_err_t err = CU_OK;
    cu_err_checkr(xlist_flush(list, free_fnc));
    free(list);

    return err;
}