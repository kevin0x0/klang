#ifndef KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H
#define KEVCC_PARGEN_INCLUDE_LR_COLLECTION_H

#include "pargen/include/lr/item.h"
#include "utils/include/set/bitset.h"

typedef struct tagKevLRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  size_t itemset_no;
  KevBitSet** firsts;
} KevLRCollection;

typedef struct tagKevLRActionEntry {
  size_t info;
  int action;
} KevLRActionEntry;

typedef struct tagKevLRAction {
  KevLRActionEntry** action;
  size_t itemset_no;
  size_t symbol_no;
} KevLRAction;

KevLRCollection* kev_lalr_generate(KevSymbol* start, KevSymbol** lookahead, size_t length);
void kev_lr_compute_first(KevBitSet** firsts, KevSymbol* symbol, size_t epsilon);
KevBitSet** kev_lr_compute_first_array(KevSymbol** symbols, size_t symbol_no, size_t terminal_no);
void kev_lr_destroy_first_array(KevBitSet** firsts, size_t size);
KevSymbol* kev_lr_augment(KevSymbol* start);
KevBitSet* kev_lr_symbols_to_bitset(KevSymbol** symbols, size_t length);

#endif
