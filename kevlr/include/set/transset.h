#ifndef KEVCC_KEVLR_INCLUDE_SET_TRANSSET_H
#define KEVCC_KEVLR_INCLUDE_SET_TRANSSET_H

#include "kevlr/include/collection.h"
#include "utils/include/array/karray.h"

typedef struct tagKlrTransSet {
  KArray symbols;
  KlrItemSet** targets;
} KlrTransSet;

bool klr_transset_init(KlrTransSet* transset, size_t nsymbol);
KlrTransSet* klr_transset_create(size_t nsymbol);
void klr_transset_destroy(KlrTransSet* transset);
void klr_transset_delete(KlrTransSet* transset);

static inline bool klr_transset_insert(KlrTransSet* transset, KlrSymbol* symbol, KlrItemSet* target);
static inline KlrItemSet* klr_transset_search(KlrTransSet* transset, KlrSymbol* symbol);
static inline void klr_transset_make_empty(KlrTransSet* transset);

static inline bool klr_transset_insert(KlrTransSet* transset, KlrSymbol* symbol, KlrItemSet* target) {
  if (!karray_push_back(&transset->symbols, symbol))
    return false;
  transset->targets[symbol->index] = target;
  return true;
}

static inline KlrItemSet* klr_transset_search(KlrTransSet* transset, KlrSymbol* symbol) {
  return transset->targets[symbol->index];
}

static inline void klr_transset_make_empty(KlrTransSet* transset) {
  size_t array_size = karray_size(&transset->symbols);
  for (size_t i = 0; i < array_size; ++i) {
    KlrSymbol* symbol = (KlrSymbol*)karray_access(&transset->symbols, i);
    transset->targets[symbol->index] = NULL;
  }
  karray_make_empty(&transset->symbols);
}

#endif
