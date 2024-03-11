#ifndef KEVCC_KEVLR_INCLUDELECTION_H
#define KEVCC_KEVLR_INCLUDELECTION_H

#include "kevlr/include/itemset.h"
#include "kevlr/include/itemset_def.h"
#include "utils/include/array/karray.h"

typedef struct tagKlrCollection {
  KlrSymbol** symbols;
  size_t nsymbol;
  size_t nterminal;
  KlrItemSet** itemsets;
  size_t nitemset;
  KBitSet** firsts;
  KlrSymbol* start;
  KlrRule* start_rule;
  KlrItemPoolCollec pool;
} KlrCollection;

/* generation of lr collection */
KlrCollection* klr_collection_create_lalr(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);
KlrCollection* klr_collection_create_lr1(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);
KlrCollection* klr_collection_create_slr(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);

void klr_collection_delete(KlrCollection* collec);

/* get methods */
static inline KlrItemSet* klr_collection_get_itemset_by_index(KlrCollection* collec, size_t index);
static inline KBitSet* klr_collection_get_firstset_by_index(KlrCollection* collec, size_t index);
static inline size_t klr_collection_nitemset(KlrCollection* collec);
/* An augmented grammar nonterminal symbol is included */
static inline size_t klr_collection_nsymbol(KlrCollection* collec);
/* An augmented grammar nonterminal symbol is excluded */
static inline size_t klr_collection_nusersymbol(KlrCollection* collec);
static inline size_t klr_collection_nterminal(KlrCollection* collec);
static inline KlrSymbol** klr_collection_get_symbols(KlrCollection* collec);
static inline KlrItemSet* klr_collection_get_start_itemset(KlrCollection* collec);
static inline KlrRule* klr_collection_get_start_rule(KlrCollection* collec);


static inline KlrItemSet* klr_collection_get_itemset_by_index(KlrCollection* collec, size_t index) {
  return collec->itemsets[index];
}

static inline KBitSet* klr_collection_get_firstset_by_index(KlrCollection* collec, size_t index) {
  return collec->firsts[index];
}

static inline size_t klr_collection_nitemset(KlrCollection* collec) {
  return collec->nitemset;
}

static inline size_t klr_collection_nsymbol(KlrCollection* collec) {
  return collec->nsymbol;
}
static inline size_t klr_collection_nusersymbol(KlrCollection* collec) {
  /* start symbol should be excluded, so the actual symbol number created by user is
   * collec->nsymbol - 1. */
  return collec->nsymbol - 1;
}

static inline KlrSymbol** klr_collection_get_symbols(KlrCollection* collec) {
  return collec->symbols;
}

static inline size_t klr_collection_nterminal(KlrCollection* collec) {
  return collec->nterminal;
}

static inline KlrItemSet* klr_collection_get_start_itemset(KlrCollection* collec) {
  return collec->itemsets[0];
}

static inline KlrRule* klr_collection_get_start_rule(KlrCollection* collec) {
  return collec->start_rule;
}

#endif
