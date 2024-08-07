#ifndef _KLANG_INCLUDE_VALUE_KLSTRING_H_
#define _KLANG_INCLUDE_VALUE_KLSTRING_H_

#include "include/lang/kltypes.h"
#include "include/misc/klutils.h"
#include "include/mm/klmm.h"

#include <stdbool.h>
#include <string.h>

typedef struct tagKlStrPool KlStrPool;
typedef struct tagKlString KlString;

struct tagKlString {
  KL_DERIVE_FROM(KlGCObjectDelegate, _gcbase_);
  KlUnsigned length;
  size_t hash;
  KlString* next;
  char content[];
};

struct tagKlStrPool {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlMM* klmm;
  KlString** array;
  size_t capacity;
  size_t size;
};


KlStrPool* klstrpool_create(KlMM* klmm, size_t capacity);
void klstrpool_destroy(KlStrPool* strpool);

KlString* klstrpool_new_string(KlStrPool* strpool, const char* str);
KlString* klstrpool_new_string_buf(KlStrPool* strpool, const char* buf, size_t buflen);

KlString* klstrpool_string_concat_cstyle(KlStrPool* strpool, const char* str1, const char* str2);
KlString* klstrpool_string_concat(KlStrPool* strpool, const KlString* str1, const KlString* str2);


static inline KlMM* klstrpool_getmm(const KlStrPool* strpool) {
  return strpool->klmm;
}

static inline size_t klstring_hash(const KlString* klstr);
static inline const char* klstring_content(const KlString* klstr);
static inline size_t klstring_size(const KlString* klstr);
static inline size_t klstring_length(const KlString* klstr);

static inline int klstring_compare(const KlString* str1, const KlString* str2);


static inline size_t klstring_hash(const KlString* klstr) {
  return klstr->hash;
}

static inline const char* klstring_content(const KlString* klstr) {
  return klstr->content;
}

static inline size_t klstring_size(const KlString* klstr) {
  return sizeof (KlString) + 1 + klstring_length(klstr);
}

static inline size_t klstring_length(const KlString* klstr) {
  return klstr->length;
}

static inline int klstring_compare(const KlString* str1, const KlString* str2) {
  return str1 == str2 ? 0 : strcmp(klstring_content(str1), klstring_content(str2));
}
#endif
