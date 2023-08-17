#ifndef KEVCC_PARGEN_INCLUDE_LR_ITEM_DEF_H
#define KEVCC_PARGEN_INCLUDE_LR_ITEM_DEF_H

#include "pargen/include/lr/rule.h"
#include "utils/include/set/bitset.h"

typedef struct tagKevKernelItem {
  KevRule* rule;
  KevBitSet* lookahead;
  struct tagKevKernelItem* next;
  size_t dot;
} KevKernelItem;

typedef struct tagKevNonKernelItem {
  KevSymbol* head;
  KevBitSet* lookahead;
  struct tagKevNonKernelItem* next;
} KevNonKernelItem;

struct tagKevItemSet;
typedef struct tagKevItemSetGoto {
  KevSymbol* symbol;
  struct tagKevItemSet* itemset;
  struct tagKevItemSetGoto* next;
} KevItemSetGoto;

typedef struct tagKevItemSet {
  KevKernelItem* items;
  KevItemSetGoto* gotos;
  size_t id;
} KevItemSet;

#endif
