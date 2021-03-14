#ifndef _CUTILS_WXP_H_
#define _CUTILS_WXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cutils.h"

cu_err_t wxp(const char* words, int* argc, char*** argv);

#ifdef __cplusplus
}
#endif

#endif