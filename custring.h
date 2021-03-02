#ifndef _CUTILS_STRING_H
#define _CUTILS_STRING_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool custreq(const char* str1, const char* str2);
bool custrneq(const char* str1, const char* str2, size_t n);
bool custrsw(const char* str1, const char* str2);
bool custrew(const char* str1, const char* str2);
bool custrn_is_digit_only(const char* str, size_t n);
size_t custrn_chrcnt(const char* str, char chr, size_t n);
char** custrsplit(const char* str, const char chr, size_t* out_len);

#endif