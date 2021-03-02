#ifndef _CUTILS_H_
#define _CUTILS_H_

#define cufree_list_(list, len, item_free_fnc) \
    ({ for(size_t i = 0; i < len; i++) { item_free_fnc(list[i]); } free(list); })

#define cufree_list(list, len) cufree_list_(list, len, free)

#endif