#ifndef _KLANG_INCLUDE_CODE_KLCONTBL_H_
#define _KLANG_INCLUDE_CODE_KLCONTBL_H_

#include "include/ast/klast.h"
#include "deps/k/include/array/karray.h"
#include "include/code/klcodeval.h"

typedef struct tagKlConEntry KlConEntry;
struct tagKlConEntry {
  KlConstant con;
  KlConEntry* next;
  size_t hash;
  KlCIdx index;
};

typedef struct tagKlConTbl {
  KlConEntry** array;   /* hash buckets */
  KArray entries;       /* all entries */
  size_t capacity;
  size_t size;
  KlStrTbl* strtbl;
} KlConTbl;


bool klcontbl_init(KlConTbl* contbl, size_t capacity, KlStrTbl* strtbl);
void klcontbl_destroy(KlConTbl* contbl);
KlConTbl* klcontbl_create(size_t capacity, KlStrTbl* strtbl);
void klcontbl_delete(KlConTbl* contbl);

static inline size_t klcontbl_size(KlConTbl* contbl);
static inline KlCIdx klcontbl_nextindex(KlConTbl* contbl);

void klcontbl_setconstants(KlConTbl* contbl, KlConstant* constants);

KlConEntry* klcontbl_insert(KlConTbl* contbl, KlConstant* con);
KlConEntry* klcontbl_search(KlConTbl* contbl, KlConstant* con);
KlConEntry* klcontbl_get(KlConTbl* contbl, KlConstant* con);

static inline KlConEntry* klcontbl_getbyindex(KlConTbl* contbl, KlCIdx index) {
  return klcast(KlConEntry*, karray_access(&contbl->entries, index));
}

static inline size_t klcontbl_size(KlConTbl* contbl) {
  return contbl->size;
}

static inline KlCIdx klcontbl_nextindex(KlConTbl* contbl) {
  return contbl->size;
}

#endif
