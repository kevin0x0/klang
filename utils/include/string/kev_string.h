#ifndef KEVCC_UTILS_INCLUDE_STRING_KEV_STRING_H
#define KEVCC_UTILS_INCLUDE_STRING_KEV_STRING_H
#include "utils/include/general/global_def.h"

size_t kev_str_len(const char* str);
char* kev_str_copy(const char* str);
char* kev_str_concat(const char* str1, const char* str2);
char* kev_str_copy_len(const char* str, size_t len);
char* kev_str_concat_len(const char* str1, const char* str2, size_t len1, size_t len2);
char* kev_trunc_leaf(const char* path);
static inline size_t kev_str_prefix(const char* prefix, const char* str);

static inline size_t kev_str_prefix(const char* prefix, const char* str) {
  size_t i = 0;
  while (prefix[i] == str[i] && prefix[i] != '\0') ++i;
  return i;
}

#endif
