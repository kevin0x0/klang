#ifndef KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H
#define KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H

#include "pargen/include/lr/itemset.h"
#include "utils/include/array/addr_array.h"

typedef struct tagKevLRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
  KevSymbol* start;
  KevRule* start_rule;
} KevLRCollection;

/* generation of lr collection */
KevLRCollection* kev_lr_collection_create_lalr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_lr1(KevSymbol* start, KevSymbol** lookahead, size_t la_len);
KevLRCollection* kev_lr_collection_create_slr(KevSymbol* start, KevSymbol** lookahead, size_t la_len);

void kev_lr_collection_delete(KevLRCollection* collec);

/* get methods */
static inline KevItemSet* kev_lr_collection_get_itemset_by_index(KevLRCollection* collec, size_t index);
static inline KevBitSet* kev_lr_collection_get_firstset_by_index(KevLRCollection* collec, size_t index);
static inline size_t kev_lr_collection_get_itmeset_no(KevLRCollection* collec);
/* A augmented grammar nonterminal symbol is included */
static inline size_t kev_lr_collection_get_symbol_no(KevLRCollection* collec);
/* A augmented grammar nonterminal symbol is excluded */
static inline size_t kev_lr_collection_get_user_symbol_no(KevLRCollection* collec);
static inline size_t kev_lr_collection_get_terminal_no(KevLRCollection* collec);
static inline KevSymbol** kev_lr_collection_get_symbols(KevLRCollection* collec);


static inline KevItemSet* kev_lr_collection_get_itemset_by_index(KevLRCollection* collec, size_t index) {
  return collec->itemsets[index];
}

static inline KevBitSet* kev_lr_collection_get_firstset_by_index(KevLRCollection* collec, size_t index) {
  return collec->firsts[index];
}

static inline size_t kev_lr_collection_get_itmeset_no(KevLRCollection* collec) {
  return collec->itemset_no;
}

static inline size_t kev_lr_collection_get_symbol_no(KevLRCollection* collec) {
  return collec->symbol_no;
}
static inline size_t kev_lr_collection_get_user_symbol_no(KevLRCollection* collec) {
  /* start symbol is excluded, so the actual symbol number created by user is
   * collec->symbol_no - 1. */
  return collec->symbol_no - 1;
}

static inline KevSymbol** kev_lr_collection_get_symbols(KevLRCollection* collec) {
  return collec->symbols;
}

static inline size_t kev_lr_collection_get_terminal_no(KevLRCollection* collec) {
  return collec->terminal_no;
}

#endif
