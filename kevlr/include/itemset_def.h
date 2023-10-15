#ifndef KEVCC_KEVLR_INCLUDE_ITEMSET_DEF_H
#define KEVCC_KEVLR_INCLUDE_ITEMSET_DEF_H

#include "kevlr/include/rule.h"
#include "utils/include/array/karray.h"
#include "utils/include/set/bitset.h"

typedef struct tagKlrItem {
  KlrRule* rule;
  KBitSet* lookahead;
  struct tagKlrItem* next;
  size_t dot;
} KlrItem;

struct tagKlrItemSet;
typedef struct tagKlrItemSetTransition {
  KlrSymbol* symbol;
  struct tagKlrItemSet* target;
  struct tagKlrItemSetTransition* next;
} KlrItemSetTransition;

typedef struct tagKlrItemSet {
  KlrItem* items;
  KlrItemSetTransition* trans;
  KlrID id;
} KlrItemSet;

typedef struct tagKlrItemSetClosure {
  KArray* symbols;
  KBitSet** lookaheads;
} KlrItemSetClosure;


#endif
