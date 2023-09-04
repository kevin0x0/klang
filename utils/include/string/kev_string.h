#ifndef KEVCC_UTILS_INCLUDE_STRING_KEV_STRING_H
#define KEVCC_UTILS_INCLUDE_STRING_KEV_STRING_H
#include "utils/include/general/global_def.h"

size_t kev_str_len(char* str);
char* kev_str_copy(char* str);
char* kev_str_concat(char* str1, char* str2);
char* kev_str_copy_len(char* str, size_t len);
char* kev_str_concat_len(char* str1, char* str2, size_t len1, size_t len2);

#endif
