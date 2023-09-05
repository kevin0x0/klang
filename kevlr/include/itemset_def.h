#ifndef KEVCC_KEVLR_INCLUDE_ITEMSET_DEF_H
#define KEVCC_KEVLR_INCLUDE_ITEMSET_DEF_H

#include "kevlr/include/rule.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/set/bitset.h"

typedef struct tagKevItem {
  KevRule* rule;
  KevBitSet* lookahead;
  struct tagKevItem* next;
  size_t dot;
} KevItem;

struct tagKevItemSet;
typedef struct tagKevItemSetGoto {
  KevSymbol* symbol;
  struct tagKevItemSet* itemset;
  struct tagKevItemSetGoto* next;
} KevItemSetGoto;

typedef struct tagKevItemSet {
  KevItem* items;
  KevItemSetGoto* gotos;
  size_t id;
} KevItemSet;

typedef struct tagKevItemSetClosure {
  KevAddrArray* symbols;
  KevBitSet** lookaheads;
} KevItemSetClosure;


#endif
