#ifndef KEVCC_KEVLR_INCLUDELECTION_H
#define KEVCC_KEVLR_INCLUDELECTION_H

#include "kevlr/include/itemset.h"
#include "kevlr/include/itemset_def.h"
#include "utils/include/array/karray.h"

typedef struct tagKlrCollection {
  KlrSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KlrItemSet** itemsets;
  size_t itemset_no;
  KBitSet** firsts;
  KlrSymbol* start;
  KlrRule* start_rule;
} KlrCollection;

/* generation of lr collection */
KlrCollection* klr_collection_create_lalr(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);
KlrCollection* klr_collection_create_lr1(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);
KlrCollection* klr_collection_create_slr(KlrSymbol* start, KlrSymbol** lookahead, size_t la_len);

void klr_collection_delete(KlrCollection* collec);

/* get methods */
static inline KlrItemSet* klr_collection_get_itemset_by_index(KlrCollection* collec, size_t index);
static inline KBitSet* klr_collection_get_firstset_by_index(KlrCollection* collec, size_t index);
static inline size_t klr_collection_get_itemset_no(KlrCollection* collec);
/* An augmented grammar nonterminal symbol is included */
static inline size_t klr_collection_get_symbol_no(KlrCollection* collec);
/* An augmented grammar nonterminal symbol is excluded */
static inline size_t klr_collection_get_user_symbol_no(KlrCollection* collec);
static inline size_t klr_collection_get_terminal_no(KlrCollection* collec);
static inline KlrSymbol** klr_collection_get_symbols(KlrCollection* collec);
static inline KlrItemSet* klr_collection_get_start_itemset(KlrCollection* collec);
static inline KlrRule* klr_collection_get_start_rule(KlrCollection* collec);


static inline KlrItemSet* klr_collection_get_itemset_by_index(KlrCollection* collec, size_t index) {
  return collec->itemsets[index];
}

static inline KBitSet* klr_collection_get_firstset_by_index(KlrCollection* collec, size_t index) {
  return collec->firsts[index];
}

static inline size_t klr_collection_get_itemset_no(KlrCollection* collec) {
  return collec->itemset_no;
}

static inline size_t klr_collection_get_symbol_no(KlrCollection* collec) {
  return collec->symbol_no;
}
static inline size_t klr_collection_get_user_symbol_no(KlrCollection* collec) {
  /* start symbol should be excluded, so the actual symbol number created by user is
   * collec->symbol_no - 1. */
  return collec->symbol_no - 1;
}

static inline KlrSymbol** klr_collection_get_symbols(KlrCollection* collec) {
  return collec->symbols;
}

static inline size_t klr_collection_get_terminal_no(KlrCollection* collec) {
  return collec->terminal_no;
}

static inline KlrItemSet* klr_collection_get_start_itemset(KlrCollection* collec) {
  return collec->itemsets[0];
}

static inline KlrRule* klr_collection_get_start_rule(KlrCollection* collec) {
  return collec->start_rule;
}

#endif
