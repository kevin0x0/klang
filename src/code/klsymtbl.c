#include "include/code/klsymtbl.h"
#include "include/cst/klstrtbl.h"
#include <stdlib.h>
#include <string.h>


static inline void klsymtbl_node_insert(KlSymbol* insertpos, KlSymbol* elem);
static inline void klsymtbl_init_head_tail(KlSymbol* head, KlSymbol* tail);

static inline void klsymtbl_node_insert(KlSymbol* insertpos, KlSymbol* node) {
  node->prev = insertpos->prev;
  insertpos->prev->next = node;
  node->next = insertpos;
  insertpos->prev = node;
}

static inline void klsymtbl_init_head_tail(KlSymbol* head, KlSymbol* tail) {
  head->next = tail;
  tail->prev = head;
  head->prev = NULL;
  tail->next = NULL;
}


inline static size_t klsymtbl_hashing(KlStrTbl* strtbl, KlStrDesc name) {
  char* str = klstrtbl_getstring(strtbl, name.id);
  char* end = str + name.length;
  size_t hash = 0;
  while (str != end)
    hash = (*str++) + (hash << 6) + (hash << 16) - hash;
  return hash;
}

static void klsymtbl_rehash(KlSymTbl* symtbl, KlSymbol** new_array, size_t new_capacity) {
  size_t mask = new_capacity - 1;
  KlSymbol* symbol = symtbl->head.next;
  KlSymbol* end = &symtbl->tail;

  KlSymbol tmphead;
  KlSymbol tmptail;
  klsymtbl_init_head_tail(&tmphead, &tmptail);

  while (symbol != end) {
    KlSymbol* tmp = symbol->next;
    size_t index = symbol->hash & mask;
    if (!new_array[index]) {
      /* this means 'symbol' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
      new_array[index] = symbol;
      klsymtbl_node_insert(tmphead.next, symbol);
    } else {
      klsymtbl_node_insert(new_array[index]->next, symbol);
    }
    symbol = tmp;
  }

  tmphead.next->prev = &symtbl->head;
  symtbl->head.next = tmphead.next;
  tmptail.prev->next = &symtbl->tail;
  symtbl->tail.prev = tmptail.prev;

  free(symtbl->array);
  symtbl->array = new_array;
  symtbl->capacity = new_capacity;
}

static bool klsymtbl_expand(KlSymTbl* symtbl) {
  size_t new_capacity = symtbl->capacity << 1;
  KlSymbol** new_array = (KlSymbol**)malloc(new_capacity * sizeof (KlSymbol*));
  if (kl_unlikely(!new_array)) return false;
  for (size_t i = 0; i < new_capacity; ++i)
    new_array[i] = NULL;
  klsymtbl_rehash(symtbl, new_array, new_capacity);
  return true;
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

  klsymtbl_init_head_tail(&symtbl->head, &symtbl->tail);
  
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
  if (symtbl->size != 0)
    klsymbolpool_dealloclist(symtbl->symbolpool, symtbl->head.next, symtbl->tail.prev);
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
  KlSymbol* end = klsymtbl_iter_end(reftbl);
  while (symbol != end) {
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
  if (!symtbl->array[index]) {
    symtbl->array[index] = newsymbol;
    klsymtbl_node_insert(symtbl->head.next, newsymbol);
  } else {
    klsymtbl_node_insert(symtbl->array[index]->next, newsymbol);
  }
  symtbl->size++;
  return newsymbol;
}

KlSymbol* klsymtbl_search(KlSymTbl* symtbl, KlStrDesc name) {
  size_t hash = klsymtbl_hashing(symtbl->strtbl, name);
  size_t mask = symtbl->capacity - 1;
  size_t index = mask & hash;
  KlSymbol* symbol = symtbl->array[index];
  if (!symbol) return NULL;
  KlSymbol* end = klsymtbl_iter_end(symtbl);
  do {
    if (hash == symbol->hash && symbol->name.length == name.length &&
        0 == strncmp(klstrtbl_getstring(symtbl->strtbl, symbol->name.id),
                     klstrtbl_getstring(symtbl->strtbl, name.id), name.length)) {
      return symbol;
    }
    symbol = klsymtbl_iter_next(symtbl, symbol);
  } while (symbol != end && ((symbol->hash & mask) == index));
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
    klsymtbl_init_head_tail(&symtbl->head, &symtbl->tail);
    return symtbl;
  }
  return klsymtbl_create(4, &pool->symbolpool, strtbl, parent);
}

void klsymtblpool_dealloc(KlSymTblPool* pool, KlSymTbl* symtbl) {
  KlSymbol** array = symtbl->array;
  size_t capacity = symtbl->capacity;
  if (symtbl->size != 0)
    klsymbolpool_dealloclist(&pool->symbolpool, symtbl->head.next, symtbl->tail.prev);
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
