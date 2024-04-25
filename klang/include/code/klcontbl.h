#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCONTBL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCONTBL_H

#include "klang/include/cst/klcst_expr.h"
#include "utils/include/array/karray.h"

typedef struct tagKlConEntry KlConEntry;
struct tagKlConEntry {
  KlConstant con;
  size_t hash;
  size_t index;
  KlConEntry* next;
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
static inline size_t klcontbl_nextindex(KlConTbl* contbl);

void klcontbl_setconstants(KlConTbl* contbl, KlConstant* constants);

KlConEntry* klcontbl_insert(KlConTbl* contbl, KlConstant* con);
KlConEntry* klcontbl_search(KlConTbl* contbl, KlConstant* con);
KlConEntry* klcontbl_get(KlConTbl* contbl, KlConstant* con);

static inline KlConEntry* klcontbl_getbyindex(KlConTbl* contbl, size_t index) {
  return klcast(KlConEntry*, karray_access(&contbl->entries, index));
}

static inline size_t klcontbl_size(KlConTbl* contbl) {
  return contbl->size;
}

static inline size_t klcontbl_nextindex(KlConTbl* contbl) {
  return contbl->size;
}

#endif
