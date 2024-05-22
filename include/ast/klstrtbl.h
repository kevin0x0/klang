#ifndef _KLANG_INCLUDE_PARSE_KLSTRTBL_H_
#define _KLANG_INCLUDE_PARSE_KLSTRTBL_H_

#include "include/misc/klutils.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define KLSTRTAB_EXTRA    (4096)

typedef struct tagKlStrDesc {
  unsigned id;
  unsigned length;
} KlStrDesc;          /* string descriptor */

typedef struct tagKlStrTbl {
  char* stack;
  char* curr;
  char* end;
} KlStrTbl;

KlStrTbl* klstrtbl_create(void);
void klstrtbl_delete(KlStrTbl* strtbl);

static inline size_t klstrtbl_residual(KlStrTbl* strtbl);
static inline size_t klstrtbl_capacity(KlStrTbl* strtbl);
static inline size_t klstrtbl_size(KlStrTbl* strtbl);

char* klstrtbl_concat(KlStrTbl* strtbl, KlStrDesc left, KlStrDesc right);

char* klstrtbl_grow(KlStrTbl* strtbl, size_t extra);
static inline bool klstrtbl_checkspace(KlStrTbl* strtbl, size_t space);
/* get a pointer to the top of the stack. */
static inline char* klstrtbl_allocstring(KlStrTbl* strtbl, size_t size);
static inline size_t klstrtbl_pushstring(KlStrTbl* strtbl, size_t length);
static inline char* klstrtbl_newstring(KlStrTbl* strtbl, const char* str);
/* get offset(id) of a string in the string table. */
static inline size_t klstrtbl_stringid(KlStrTbl* strtbl, const char* str);
/* get string by offset(id). */
static inline char* klstrtbl_getstring(KlStrTbl* strtbl, size_t id);


static inline size_t klstrtbl_residual(KlStrTbl* strtbl) {
  return strtbl->end - strtbl->curr;
}

static inline size_t klstrtbl_capacity(KlStrTbl* strtbl) {
  return strtbl->end - strtbl->stack;
}

static inline size_t klstrtbl_size(KlStrTbl* strtbl) {
  return strtbl->curr - strtbl->stack;
}

static inline bool klstrtbl_checkspace(KlStrTbl* strtbl, size_t space) {
  return klstrtbl_residual(strtbl) >= space;
}

static inline char* klstrtbl_allocstring(KlStrTbl* strtbl, size_t size) {
  if (kl_unlikely(!klstrtbl_checkspace(strtbl, size)))
    return klstrtbl_grow(strtbl, size);
  return strtbl->curr;
}

static inline size_t klstrtbl_pushstring(KlStrTbl* strtbl, size_t length) {
  size_t id = strtbl->curr - strtbl->stack;
  strtbl->curr += length;
  return id;
}

static inline char* klstrtbl_newstring(KlStrTbl* strtbl, const char* str) {
  int len = strlen(str);
  char* mystr = klstrtbl_allocstring(strtbl, len);
  if (kl_unlikely(!mystr)) return mystr;
  memcpy(mystr, str, len * sizeof (char));
  klstrtbl_pushstring(strtbl, len);
  return mystr;
}

static inline size_t klstrtbl_stringid(KlStrTbl* strtbl, const char* str) {
  kl_assert(str >= strtbl->stack && str <= strtbl->curr, "klstrtbl_stringid(): invalid string");

  return str - strtbl->stack;
}

static inline char* klstrtbl_getstring(KlStrTbl* strtbl, size_t id) {
  kl_assert(id <= klstrtbl_size(strtbl), "klstrtbl_stringid(): invalid string");

  return strtbl->stack + id;
}

#endif
