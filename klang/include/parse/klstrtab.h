#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLSTRTAB_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLSTRTAB_H

#include "klang/include/misc/klutils.h"
#include <stdbool.h>
#include <stddef.h>

#define KLSTRTAB_EXTRA    (4096)

typedef struct tagKlStrDesc {
  size_t id;
  size_t length;
} KlStrDesc;          /* string descriptor */

typedef struct tagKlStrTab {
  char* stack;
  char* curr;
  char* end;
} KlStrTab;

KlStrTab* klstrtab_create(void);
void klstrtab_delete(KlStrTab* strtab);

static inline size_t klstrtab_residual(KlStrTab* strtab);
static inline size_t klstrtab_capacity(KlStrTab* strtab);
static inline size_t klstrtab_size(KlStrTab* strtab);

char* klstrtab_concat(KlStrTab* strtab, KlStrDesc left, KlStrDesc right);

char* klstrtab_grow(KlStrTab* strtab, size_t extra);
static inline bool klstrtab_checkspace(KlStrTab* strtab, size_t space);
/* get a pointer to the top of the stack. */
static inline char* klstrtab_allocstring(KlStrTab* strtab, size_t size);
static inline size_t klstrtab_pushstring(KlStrTab* strtab, size_t length);
/* get offset(id) of a string in the string table. */
static inline size_t klstrtab_stringid(KlStrTab* strtab, char* str);
/* get string by offset(id). */
static inline char* klstrtab_getstring(KlStrTab* strtab, size_t id);


static inline size_t klstrtab_residual(KlStrTab* strtab) {
  return strtab->end - strtab->curr;
}

static inline size_t klstrtab_capacity(KlStrTab* strtab) {
  return strtab->end - strtab->stack;
}

static inline size_t klstrtab_size(KlStrTab* strtab) {
  return strtab->curr - strtab->stack;
}

static inline bool klstrtab_checkspace(KlStrTab* strtab, size_t space) {
  return klstrtab_residual(strtab) >= space;
}

static inline char* klstrtab_allocstring(KlStrTab* strtab, size_t size) {
  if (kl_unlikely(!klstrtab_checkspace(strtab, size)))
    return klstrtab_grow(strtab, size);
  return strtab->curr;
}

static inline size_t klstrtab_pushstring(KlStrTab* strtab, size_t length) {
  size_t id = strtab->curr - strtab->stack;
  strtab->curr += length;
  return id;
}

static inline size_t klstrtab_stringid(KlStrTab* strtab, char* str) {
  kl_assert(str >= strtab->stack && str <= strtab->curr, "klstrtab_stringid(): invalid string");

  return str - strtab->stack;
}

static inline char* klstrtab_getstring(KlStrTab* strtab, size_t id) {
  kl_assert(id <= klstrtab_size(strtab), "klstrtab_stringid(): invalid string");

  return strtab->stack + id;
}

#endif
