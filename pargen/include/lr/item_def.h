#ifndef KEVCC_PARGEN_INCLUDE_LR_ITEM_DEF_H
#define KEVCC_PARGEN_INCLUDE_LR_ITEM_DEF_H

#include "pargen/include/lr/rule.h"
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

#endif
