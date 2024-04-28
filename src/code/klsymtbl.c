#include "include/code/klsymtbl.h"
#include "include/cst/klstrtbl.h"
#include <stdlib.h>
#include <string.h>

inline static size_t klsymtbl_hashing(KlStrTbl* strtbl, KlStrDesc name) {
  char* str = klstrtbl_getstring(strtbl, name.id);
  char* end = str + name.length;
  size_t hash = 0;
  while (str != end)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}

static void klsymtbl_rehash(KlSymTbl* to, KlSymTbl* from) {
  size_t from_capacity = from->capacity;
  size_t to_capacity = to->capacity;
  KlSymbol** from_array = from->array;
  KlSymbol** to_array = to->array;
  size_t mask = to_capacity - 1;
  for (size_t i = 0; i < from_capacity; ++i) {
    KlSymbol* symbol = from_array[i];
    while (symbol) {
      KlSymbol* tmp = symbol->next;
      size_t hashval = symbol->hash;
      size_t index = hashval & mask;
      symbol->next = to_array[index];
      to_array[index] = symbol;
      symbol = tmp;
    }
  }
  to->size = from->size;
  free(from->array);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool klsymtbl_expand(KlSymTbl* symtbl) {
  KlSymTbl newsymtbl;
  if (kl_unlikely(!klsymtbl_init(&newsymtbl, symtbl->symbolpool, symtbl->capacity << 1, symtbl->strtbl, symtbl->parent)))
    return false;
  klsymtbl_rehash(&newsymtbl, symtbl);
  *symtbl = newsymtbl;
  return true;
}

static void klsymtbl_bucket_free(KlSymbolPool* symbolpool, KlSymbol* bucket) {
  while (bucket) {
    KlSymbol* tmp = bucket->next;
    klsymbolpool_dealloc(symbolpool, bucket);
    bucket = tmp;
  }
}

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool klsymtbl_init(KlSymTbl* symtbl, KlSymbolPool* pool, size_t capacity, KlStrTbl* strtbl, KlSymTbl* parent) {
  if (!symtbl) return false;

  /* TODO: make sure capacity is power of 2 */
  capacity = pow_of_2_above(capacity);
  KlSymbol** array = (KlSymbol**)malloc(sizeof (KlSymbol*) * capacity);
  if (kl_unlikely(!array)) return false;
  for (size_t i = 0; i < capacity; ++i) {
    array[i] = NULL;
  }
  
  symtbl->array = array;
  symtbl->capacity = capacity;
  symtbl->size = 0;
  symtbl->parent = parent;
  symtbl->strtbl = strtbl;
  symtbl->symbolpool = pool;
  symtbl->info.referenced = false;
  return true;
}

void klsymtbl_destroy(KlSymTbl* symtbl) {
  if (kl_unlikely(!symtbl)) return;

  KlSymbol** array = symtbl->array;
  size_t capacity = symtbl->capacity;
  for (size_t i = 0; i < capacity; ++i)
    klsymtbl_bucket_free(symtbl->symbolpool, array[i]);
  free(array);
}

KlSymTbl* klsymtbl_create(size_t capacity, KlSymbolPool* pool, KlStrTbl* strtbl, KlSymTbl* parent) {
  KlSymTbl* symtbl = (KlSymTbl*)malloc(sizeof (KlSymTbl));
  if (kl_unlikely(!symtbl || !klsymtbl_init(symtbl, pool, capacity, strtbl, parent))) {
    free(strtbl);
  }
  return symtbl;
}

void klsymtbl_delete(KlSymTbl* symtbl) {
  klsymtbl_destroy(symtbl);
  free(symtbl);
}

void klreftbl_setrefinfo(KlSymTbl* reftbl, KlRefInfo* refinfo) {
  KlSymbol* symbol = klsymtbl_iter_begin(reftbl);
  while (symbol) {
    refinfo[symbol->attr.idx].on_stack = symbol->attr.refto->attr.kind == KLVAL_STACK;
    refinfo[symbol->attr.idx].index = symbol->attr.refto->attr.idx;
    symbol = klsymtbl_iter_next(reftbl, symbol);
  }
}

KlSymbol* klsymtbl_insert(KlSymTbl* symtbl, KlStrDesc name) {
  if (kl_unlikely(symtbl->size >= symtbl->capacity && !klsymtbl_expand(symtbl)))
    return NULL;

  KlSymbol* newsymbol = klsymbolpool_alloc(symtbl->symbolpool);
  if (kl_unlikely(!newsymbol)) return NULL;

  size_t hash = klsymtbl_hashing(symtbl->strtbl, name);
  size_t index = (symtbl->capacity - 1) & hash;
  newsymbol->name = name;
  newsymbol->hash = hash;
  newsymbol->next = symtbl->array[index];
  symtbl->array[index] = newsymbol;
  symtbl->size++;
  return newsymbol;
}

KlSymbol* klsymtbl_search(KlSymTbl* symtbl, KlStrDesc name) {
  size_t hash = klsymtbl_hashing(symtbl->strtbl, name);
  size_t index = (symtbl->capacity - 1) & hash;
  KlSymbol* symbol = symtbl->array[index];
  for (; symbol; symbol = symbol->next) {
    if (hash == symbol->hash && symbol->name.length == name.length &&
        0 == strncmp(klstrtbl_getstring(symtbl->strtbl, symbol->name.id),
                klstrtbl_getstring(symtbl->strtbl, name.id), name.length)) {
      return symbol;
    }
  }

  return NULL;
}



void klsymtblpool_init(KlSymTblPool* pool) {
  pool->tables = NULL;
  pool->symbolpool.symbols = NULL;
}

void klsymtblpool_destroy(KlSymTblPool* pool) {
  KlSymTbl* tbl = pool->tables;
  while (tbl) {
    KlSymTbl* tmp = tbl->next;
    free(tbl->array);
    free(tbl);
    tbl = tmp;
  }
  KlSymbol* symbol = pool->symbolpool.symbols;
  while (symbol) {
    KlSymbol* tmp = symbol->next;
    free(symbol);
    symbol = tmp;
  }
}

KlSymTbl* klsymtblpool_alloc(KlSymTblPool* pool, KlStrTbl* strtbl, KlSymTbl* parent) {
  if (kl_likely(pool->tables)) {
    KlSymTbl* symtbl = pool->tables;
    pool->tables = symtbl->next;
    symtbl->parent = parent;
    symtbl->strtbl = strtbl;
    return symtbl;
  }
  return klsymtbl_create(4, &pool->symbolpool, strtbl, parent);
}

void klsymtblpool_dealloc(KlSymTblPool* pool, KlSymTbl* symtbl) {
  KlSymbol** array = symtbl->array;
  size_t capacity = symtbl->capacity;
  for (size_t i = 0; i < capacity; ++i)
    klsymtbl_bucket_free(symtbl->symbolpool, array[i]);
  if (symtbl->capacity >= 32) {
    KlSymbol** arr = realloc(array, 16 * sizeof (KlSymbol*));
    if (arr) {
      symtbl->array = arr;
      symtbl->capacity = 16;
    }
  }
  capacity = symtbl->capacity;
  array = symtbl->array;
  for (size_t i = 0; i < capacity; ++i)
    array[i] = NULL;
  symtbl->size = 0;
  symtbl->next = pool->tables;
  pool->tables = symtbl;
}
