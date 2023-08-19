#ifndef KEVCC_PARGEN_INCLUDE_LR_LALR_H
#define KEVCC_PARGEN_INCLUDE_LR_LALR_H

#include "pargen/include/lr/lr.h"
#include "utils/include/set/bitset.h"

typedef struct tagKevLookaheadPropagation {
  KevBitSet* from;
  KevBitSet* to;
  struct tagKevLookaheadPropagation* next;
} KevLookaheadPropagation;

typedef struct tagKevLALRCollection {
  KevSymbol** symbols;
  size_t symbol_no;
  size_t terminal_no;
  KevItemSet** itemsets;
  KevLookaheadPropagation* propagation;
  size_t itemset_no;
  KevBitSet** firsts;
  KevSymbol* start;
  KevRule* start_rule;
} KevLALRCollection;



#endif
