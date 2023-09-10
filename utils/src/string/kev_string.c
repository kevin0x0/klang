#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>

char* kev_str_copy(const char* str) {
  if (!str) return NULL;
  size_t len = strlen(str);
  char* ret = (char*)malloc(sizeof (char) * (len + 1));
  if (!ret) return NULL;
  memcpy(ret, str, sizeof (char) * (len + 1 ));
  return ret;
}

char* kev_str_concat(const char* str1, const char* str2) {
  if (!str1 || !str2) return NULL;
  size_t len1 = strlen(str1);
  size_t len2 = strlen(str2);

  char* ret = (char*)malloc(sizeof (char) * (len1 + len2 + 1));
  if (!ret) return NULL;
  memcpy(ret, str1, sizeof (char) * len1);
  memcpy(ret + len1, str2, sizeof (char) * (len2 + 1));
  return ret;
}

size_t kev_str_len(const char* str) {
  if (!str) return 0;
  return strlen(str);
}

char* kev_str_copy_len(const char* str, size_t len) {
  if (!str) return NULL;
  char* ret = (char*)malloc(sizeof (char) * (len + 1));
  if (!ret) return NULL;
  memcpy(ret, str, sizeof (char) * len);
  ret[len] = '\0';
  return ret;
}

char* kev_str_concat_len(const char* str1, const char* str2, size_t len1, size_t len2) {
  if (!str1 || !str2) return NULL;
  char* ret = (char*)malloc(sizeof (char) * (len1 + len2 + 1));
  if (!ret) return NULL;
  memcpy(ret, str1, sizeof (char) * len1);
  memcpy(ret + len1, str2, sizeof (char) * len2);
  ret[len1 + len2] = '\0';
  return ret;
}

char* kev_trunc_leaf(const char* path) {
  size_t i = 0;
  char* cp_path = kev_str_copy(path);
  if (!cp_path) return NULL;
  char* p = cp_path - 1;
  while (*++p != '\0') {
    if (*p == '/' || *p == '\\')
      i = p - cp_path;
  }
  cp_path[i + 1] = '\0';
  return cp_path;
}
