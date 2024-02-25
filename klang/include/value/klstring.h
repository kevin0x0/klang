#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLSTRING_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLSTRING_H

#include "klang/include/mm/klmm.h"

#include <stdbool.h>
#include <string.h>

typedef struct tagKlStrPool KlStrPool;
typedef struct tagKlString KlString;

struct tagKlString {
  KlGCObject gcbase;
  KlStrPool* strpool;
  KlString* prev;
  KlString* next;
  size_t length;
  size_t hash;
  char strhead[1];
};

struct tagKlStrPool {
  KlGCObject gcbase;
  KlString head;
  KlString tail;
  KlString** array;
  size_t capacity;
  size_t size;
};


KlStrPool* klstrpool_create(KlMM* klmm, size_t capacity);
void klstrpool_destroy(KlStrPool* strpool);

KlString* klstrpool_new_string(KlStrPool* strpool, const char* str);

KlString* klstrpool_string_concat(KlStrPool* strpool, KlString* str1, KlString* str2);


static inline KlString* klstrpool_iter_begin(KlStrPool* strpool);
static inline KlString* klstrpool_iter_end(KlStrPool* strpool);
static inline KlString* klstrpool_iter_next(KlString* str);

static inline size_t klstring_hash(KlString* klstr);
static inline const char* klstring_content(KlString* klstr);
static inline size_t klstring_size(KlString* klstr);
static inline size_t klstring_length(KlString* klstr);

static inline int klstring_compare(KlString* str1, KlString* str2);


static inline KlString* klstrpool_iter_begin(KlStrPool* strpool) {
  return strpool->head.next;
}

static inline KlString* klstrpool_iter_end(KlStrPool* strpool) {
  return &strpool->tail;
}

static inline KlString* klstrpool_iter_next(KlString* str) {
  return str->next;
}



static inline size_t klstring_hash(KlString* klstr) {
  return klstr->hash;
}

static inline const char* klstring_content(KlString* klstr) {
  return klstr->strhead;
}

static inline size_t klstring_size(KlString* klstr) {
  return sizeof (KlString) + klstring_length(klstr);
}

static inline size_t klstring_length(KlString* klstr) {
  return klstr->length;
}

static inline int klstring_compare(KlString* str1, KlString* str2) {
  return str1 == str2 ? 0 : strcmp(klstring_content(str1), klstring_content(str2));
}
#endif
