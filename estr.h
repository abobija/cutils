#ifndef _CUTILS_STRING_H
#define _CUTILS_STRING_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Concatenate optional number of strings. User need to release the resulting string with a free function.
 *        Make sure that no one of the strings are NULL, otherwise concatenation will stop on the first NULL.
 * @return Pointer to result string or NULL on failure (no memory)
 */
#define estrcat(...) _estrcat(__VA_ARGS__, NULL)

/**
 * @brief Check if two strings are equal
 * @param str1 First string
 * @param str2 Second string
 * @return true if first and second strings are equal
 */
bool estreq(const char* str1, const char* str2);
bool estrneq(const char* str1, const char* str2, size_t n);

/**
 * @brief Check if one string starts with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string starts with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool estrsw(const char* str1, const char* str2);

/**
 * @brief Check if one string ends with another
 * @param str1 First string
 * @param str2 Second string
 * @return true if first string ends with second one.
 *         In special cases when second or both strings are empty, function will return false
 */
bool estrew(const char* str1, const char* str2);
bool estrn_is_digit_only(const char* str, size_t n);
size_t estrn_chrcnt(const char* str, char chr, size_t n);
char** estrsplit(const char* str, const char chr, size_t* out_len);

/**
 * @brief Don't use this function. Use estrcat macro instead.
 */
char* _estrcat(const char* str, ...);

/**
 * @brief Http url string encoding.
 *        Resulting encoded string of this functions needs to be freed with free function
 * @param str String which needs to be encoded
 * @return Pointer to encoded string
 */
char* estr_url_encode(const char* str);

#endif