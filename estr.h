#ifndef _CUTILS_STRING_H
#define _CUTILS_STRING_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool estreq(const char* str1, const char* str2);
bool estrneq(const char* str1, const char* str2, size_t n);
bool estrsw(const char* str1, const char* str2);
bool estrew(const char* str1, const char* str2);
bool estrn_is_digit_only(const char* str, size_t n);
size_t estrn_chrcnt(const char* str, char chr, size_t n);
char** estrsplit(const char* str, const char chr, size_t* out_len);

#endif