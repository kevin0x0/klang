#ifndef KEVCC_UTILS_INCLUDE_KSTRING_KSTRING_H
#define KEVCC_UTILS_INCLUDE_KSTRING_KSTRING_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>


/* string header */
typedef struct tagKString {
  size_t len;
  char string[1];
} KString;

KString* kstring_create(const char* cstr);
KString* kstring_create_copy(const KString* kstr);
KString* kstring_create_buf(const char* cstr_begin, const char* cstr_end);
KString* kstring_create_len(size_t len);
void kstring_delete(KString* kstr);


static inline size_t kstring_get_len(const KString* kstr);
/* get the content of string */
static inline char* kstring_get_content(KString* kstr);
static inline const char* kstring_get_content_const(const KString* kstr);

/* return the max common prefix length */
static inline size_t kstring_common_prefix(const KString* kstr1, const KString* kstr2);
static inline bool kstring_is_prefix(const KString* prefix, const KString* kstr);

KString* kstring_concat(const KString* kstr1, const KString* kstr2);



static inline size_t kstring_common_prefix(const KString* kstr1, const KString* kstr2) {
  const char* str1 = kstr1->string;
  const char* str2 = kstr2->string;
  const char* str1_cmp_end = str1 + (kstr1->len < kstr2->len ? kstr1->len : kstr2->len);
  while (str1 != str1_cmp_end && *str1++ == *str2++)
    continue;
  return str1 - kstr1->string;
}

static inline bool kstring_is_prefix(const KString* prefix, const KString* kstr) {
  return kstring_common_prefix(prefix, kstr) == prefix->len;
}

static inline int kstring_compare(const KString* kstr1, const KString* kstr2);


static inline size_t kstring_get_len(const KString* kstr) {
  return kstr->len;
}

static inline char* kstring_get_content(KString* kstr) {
  return kstr->string;
}

static inline const char* kstring_get_content_const(const KString* kstr) {
  return kstr->string;
}


static inline int kstring_compare(const KString* kstr1, const KString* kstr2) {
  if (kstr1->len == kstr2->len) {
    return memcmp(kstr1, kstr2, kstr1->len);
  } else {
    return kstr1->len - kstr2->len;
  }
}

#endif
