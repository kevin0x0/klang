#ifndef KEVCC_KLANG_INCLUDE_CODE_KLSYMTBL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLSYMTBL_H

#include "klang/include/cst/klstrtbl.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/value/klref.h"
#include <stdlib.h>

typedef KlValKind KlSymKind;

typedef struct tagKlSymbol KlSymbol;

typedef struct tagKlSymbolAttr {
  KlSymKind kind;
  size_t idx;
  KlSymbol* refto;  /* if reference, this points to the referenced symbol */
} KlSymbolAttr;

struct tagKlSymbol {
  size_t hash;
  KlStrDesc name;
  KlSymbolAttr attr;
  KlSymbol* next;
};

typedef struct tagKlSymbolPool {
  KlSymbol* symbols;
} KlSymbolPool;

typedef struct tagKlSymTbl KlSymTbl;
struct tagKlSymTbl {
  KlSymbol** array;
  size_t capacity;
  size_t size;
  union {
    KlSymTbl* parent;
    KlSymTbl* next;     /* used in symtbl pool */
  };
  KlSymbolPool* symbolpool;
  KlStrTbl* strtbl;
  struct {
    size_t stkbase;
    bool referenced;
  } info;
};


bool klsymtbl_init(KlSymTbl* symtbl, KlSymbolPool* pool, size_t capacity, KlStrTbl* strtbl, KlSymTbl* parent);
void klsymtbl_destroy(KlSymTbl* symtbl);
KlSymTbl* klsymtbl_create(size_t capacity, KlSymbolPool* pool, KlStrTbl* strtbl, KlSymTbl* parent);
void klsymtbl_delete(KlSymTbl* symtbl);

void klreftbl_setrefinfo(KlSymTbl* reftbl, KlRefInfo* refinfo);

static inline KlSymbol* klsymtbl_iter_begin(KlSymTbl* symtbl);
static inline KlSymbol* klsymtbl_iter_next(KlSymTbl* symtbl, KlSymbol* symbol);

KlSymbol* klsymtbl_insert(KlSymTbl* symtbl, KlStrDesc name);
KlSymbol* klsymtbl_search(KlSymTbl* map, KlStrDesc name);
static inline KlSymTbl* klsymtbl_parent(KlSymTbl* symtbl);
static inline size_t klsymtbl_size(KlSymTbl* symtbl);

static inline KlSymbol* klsymbolpool_alloc(KlSymbolPool* pool);
static inline void klsymbolpool_dealloc(KlSymbolPool* pool, KlSymbol* symbol);

static inline KlSymbol* klsymtbl_iter_begin(KlSymTbl* symtbl) {
  for (size_t i = 0; i < symtbl->capacity; ++i) {
    if (symtbl->array[i])
      return symtbl->array[i];
  }
  return NULL;
}

static inline KlSymbol* klsymtbl_iter_next(KlSymTbl* symtbl, KlSymbol* symbol) {
  if (symbol->next) return symbol->next;
  size_t index = symbol->hash & (symtbl->capacity - 1);
  for (size_t i = index + 1; i < symtbl->capacity; ++i) {
    if (symtbl->array[i])
      return symtbl->array[i];
  }
  return NULL;
}

static inline KlSymTbl* klsymtbl_parent(KlSymTbl* symtbl) {
  return symtbl->parent;
}

static inline size_t klsymtbl_size(KlSymTbl* symtbl) {
  return symtbl->size;
}

static inline KlSymbol* klsymbolpool_alloc(KlSymbolPool* pool) {
  if (kl_likely(pool->symbols)) {
    KlSymbol* symbol = pool->symbols;
    pool->symbols = symbol->next;
    return symbol;
  }
  return (KlSymbol*)malloc(sizeof (KlSymbol));
}

static inline void klsymbolpool_dealloc(KlSymbolPool* pool, KlSymbol* symbol) {
  symbol->next = pool->symbols;
  pool->symbols = symbol;
}


typedef struct tagKlSymTblPool {
  KlSymTbl* tables;
  KlSymbolPool symbolpool;
} KlSymTblPool;

void klsymtblpool_init(KlSymTblPool* pool);
void klsymtblpool_destroy(KlSymTblPool* pool);
KlSymTbl* klsymtblpool_alloc(KlSymTblPool* pool, KlStrTbl* strtbl, KlSymTbl* parent);
void klsymtblpool_dealloc(KlSymTblPool* pool, KlSymTbl* symtbl);


#endif
