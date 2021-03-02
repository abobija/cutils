#include "estr.h"
#include "cutils.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

bool estreq(const char* str1, const char* str2) {
    if(!str1 || !str2)
        return false;
    
    return strcmp(str1, str2) == 0;
}

bool estrneq(const char* str1, const char* str2, size_t n) {
    if(!str1 || !str2)
        return false;
    
    return strncmp(str1, str2, n) == 0;
}

bool estrsw(const char* str1, const char* str2) {
    if(!str1 || !str2) {
        return false;
    }

    size_t i = 0;
    for(; str2[i] && str1[i] && str2[i] == str1[i]; i++);
    
    return i > 0 && !str2[i];
}

bool estrew(const char* str1, const char* str2) {
	if(!str1 || !str2)
		return false;
	
	size_t len1 = strlen(str1);

	if(len1 == 0)
		return false;

	int diff = len1 - strlen(str2);
    
	if(diff < 0 || diff == len1) // str2 is bigger or empty
		return false;
	
	int i = len1 - 1;
	for(; (i - diff) >= 0 && str1[i] == str2[i - diff]; i--);

	return i < diff;
}

bool estrn_is_digit_only(const char* str, size_t n) {
    if(!str)
        return false;

    size_t len = strlen(str);
    size_t limit = n <= len ? n : len;

    for(size_t i = 0; i < limit; i++) {
        if (str[i] < '0' || str[i] > '9')
            return false;
    }

    return true;
}

size_t estrn_chrcnt(const char* str, char chr, size_t n) {
    if(!str)
        return 0;
    
	size_t cnt = 0;
    size_t len = strlen(str);
    size_t limit = n <= len ? n : len;

	for(size_t i = 0; i < limit; i++) {
		if(str[i] == chr)
			cnt++;
	}

	return cnt;
}

char** estrsplit(const char* str, const char chr, size_t* out_len) {
    if(!str || !out_len)
        return NULL;

    char* ptr = (char*) str;
    char prev = '\0';
    size_t len = 0;

    while(*ptr) {
        if(prev && ((*ptr == chr && prev != chr) || (!*(ptr + 1) && *ptr != chr))) len++;
        prev = *ptr;
        ptr++;
    }

    char** result = NULL;

    if((*out_len = len) <= 0) {
        if(!*str || *str == chr)
            return NULL;
        
        result = calloc(*out_len = 1, sizeof(char*));
        if(!result) { *out_len = 0; return NULL; }
        result[0] = strdup(str);
        if(!result[0]) { *out_len = 0; free(result); return NULL; }
        return result;
    }

    result = calloc(len, sizeof(char*));
    if(!result) { *out_len = 0; return NULL; }

    char* start = (char*) str;
    ptr = start;
    prev = '\0';
    size_t i = 0;
    
    while(*ptr) {
        if(*ptr != chr && prev == chr)
            start = ptr;
        
        bool offset_one = !*(ptr + 1) && *ptr != chr;

        if(prev && ((*ptr == chr && prev != chr) || offset_one)) {
            size_t piece_len = (offset_one ? ptr + 1 : ptr) - start;
            result[i] = malloc(piece_len + 1);
            if(!result[i]) {
                cufree_list(result, i + 1);
                *out_len = 0;
                return NULL;
            }
            memcpy(result[i], start, piece_len);
            result[i][piece_len] = '\0';
            i++;
        }

        prev = *ptr;
        ptr++;
    }

    return result;
}

char* _estrcat(const char* str, ...) {
    const char* first = str;
    size_t length = 0;
    va_list count;
    va_list copy;

    va_start(count, str);
    va_copy(copy, count);
    while(str) {
        length += strlen(str);
        str = va_arg(count, const char*);
    }
    va_end(count);

    if(length <= 0) {
        va_end(copy);
        return NULL;
    }
    
    char* res = malloc(length + 1);

    if(!res) {
        va_end(copy);
        return NULL;
    }

    size_t offset = 0;
    str = first;

    while(str) {
        size_t _len = strlen(str);

        if(_len > 0) {
            memcpy(res + offset, str, _len);
            offset += _len;
        }

        str = va_arg(copy, const char*);
    }
    va_end(copy);
    
    res[length] = '\0';

    return res;
}

char* estr_url_encode(const char* str) {
    if(!str) { return NULL; }
    static char hex[] = "0123456789abcdef";
    size_t _len = strlen(str);
    char* buf = malloc(_len * 3 + 1); // optimize?
    if(!buf) { return NULL; }
    char* pbuf = buf;

    for(size_t i = 0; i < _len; i++) {
        if (isalnum(str[i]) || strchr(".-_~", str[i])) {
            *pbuf++ = str[i];
        } else if (str[i] == ' ') {
            *pbuf++ = '+';
        } else {
            *pbuf++ = '%';
            *pbuf++ = hex[(str[i] >> 4) & 15];
            *pbuf++ = hex[str[i] & 15];
        }
    }

    *pbuf = '\0';

    return buf;
}