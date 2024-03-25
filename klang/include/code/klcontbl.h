#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCONTBL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCONTBL_H

#include "klang/include/cst/klcst_expr.h"
#include "klang/include/parse/klstrtab.h"

typedef struct tagKlConEntry KlConEntry;
struct tagKlConEntry {
  KlConstant con;
  size_t hash;
  size_t index;
  KlConEntry* next;
};

typedef struct tagKlConTbl {
  KlConEntry** array;
  size_t capacity;
  size_t size;
  KlStrTab* strtab;
} KlConTbl;


bool klcontbl_init(KlConTbl* contbl, size_t capacity, KlStrTab* strtab);
void klcontbl_destroy(KlConTbl* contbl);
KlConTbl* klcontbl_create(size_t capacity, KlStrTab* strtab);
void klcontbl_delete(KlConTbl* contbl);

KlConEntry* klcontbl_insert(KlConTbl* contbl, KlConstant* con);
KlConEntry* klcontbl_search(KlConTbl* map, KlConstant* con);
KlConEntry* klcontbl_get(KlConTbl* map, KlConstant* con);

#endif
