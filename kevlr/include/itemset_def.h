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

union tagKlrItemSet;
typedef struct tagKlrItemSetTransition {
  KlrSymbol* symbol;
  union tagKlrItemSet* target;
  struct tagKlrItemSetTransition* next;
} KlrItemSetTransition;

typedef union tagKlrItemSet {
  struct {
    KlrItem* items;
    KlrItemSetTransition* trans;
    KlrID id;
  };
  union tagKlrItemSet* next;
} KlrItemSet;

typedef struct tagKlrItemSetClosure {
  KArray* symbols;
  KBitSet** lookaheads;
} KlrItemSetClosure;


/* object pool */
typedef struct tagKlrItemSetPool {
  KlrItemSet* avlist;
} KlrItemSetPool;

typedef struct tagKlrItemPool {
  KlrItem* avlist;
} KlrItemPool;

typedef struct tagKlrItemSetTransPool {
  KlrItemSetTransition* avlist;
} KlrItemSetTransPool;

typedef struct tagKlrItemPoolCollec {
  KlrItemPool itempool;
  KlrItemSetPool itemsetpool;
  KlrItemSetTransPool itemsettranspool;
} KlrItemPoolCollec;

static inline void klr_itempool_init(KlrItemPool* pool);
static inline void klr_itemsetpool_init(KlrItemSetPool* pool);
static inline void klr_itemsettranspool_init(KlrItemSetTransPool* pool);
static inline void klr_itempoolcollec_init(KlrItemPoolCollec* pool);
static inline void klr_itempool_destroy(KlrItemPool* pool);
static inline void klr_itemsetpool_destroy(KlrItemSetPool* pool);
static inline void klr_itemsettranspool_destroy(KlrItemSetTransPool* pool);
static inline void klr_itempoolcollec_destroy(KlrItemPoolCollec* pool);

static inline void klr_itempool_init(KlrItemPool* pool) {
  pool->avlist = NULL;
}

static inline void klr_itemsetpool_init(KlrItemSetPool* pool) {
  pool->avlist = NULL;
}
static inline void klr_itemsettranspool_init(KlrItemSetTransPool* pool) {
  pool->avlist = NULL;
}

static inline void klr_itempoolcollec_init(KlrItemPoolCollec* pool) {
  klr_itempool_init(&pool->itempool);
  klr_itemsetpool_init(&pool->itemsetpool);
  klr_itemsettranspool_init(&pool->itemsettranspool);
}

static inline void klr_itempool_destroy(KlrItemPool* pool) {
  KlrItem* item = pool->avlist;
  while (item) {
    KlrItem* tmp = item->next;
    free(item);
    item = tmp;
  }
}
static inline void klr_itemsetpool_destroy(KlrItemSetPool* pool) {
  KlrItemSet* itemset = pool->avlist;
  while (itemset) {
    KlrItemSet* tmp = itemset->next;
    free(itemset);
    itemset = tmp;
  }
}

static inline void klr_itemsettranspool_destroy(KlrItemSetTransPool* pool) {
  KlrItemSetTransition* trans = pool->avlist;
  while (trans) {
    KlrItemSetTransition* tmp = trans->next;
    free(trans);
    trans = tmp;
  }
}

static inline void klr_itempoolcollec_destroy(KlrItemPoolCollec* pool) {
  klr_itempool_destroy(&pool->itempool);
  klr_itemsetpool_destroy(&pool->itemsetpool);
  klr_itemsettranspool_destroy(&pool->itemsettranspool);
}


static inline KlrItemSetTransition* klr_itemsettranspool_allocate(KlrItemSetTransPool* pool) {
  if (pool->avlist) {
    KlrItemSetTransition* retval = pool->avlist;
    pool->avlist = retval->next;
    return retval;
  }
  return (KlrItemSetTransition*)malloc(sizeof (KlrItemSetTransition));
}

static inline void klr_itemsettranspool_deallocate(KlrItemSetTransPool* pool, KlrItemSetTransition* itemsetgoto) {
  if (!itemsetgoto) return;
  itemsetgoto->next = pool->avlist;
  pool->avlist = itemsetgoto;
}

static inline KlrItem* klr_itempool_allocate(KlrItemPool* pool) {
  if (pool->avlist) {
    KlrItem* retval = pool->avlist;
    pool->avlist = retval->next;
    return retval;
  }
  return (KlrItem*)malloc(sizeof (KlrItem));
}

static inline void klr_itempool_deallocate(KlrItemPool* pool, KlrItem* item) {
  if (!item) return;
  item->next = pool->avlist;
  pool->avlist = item;
}

static inline KlrItemSet* klr_itemsetpool_allocate(KlrItemSetPool* pool) {
  if (pool->avlist) {
    KlrItemSet* retval = pool->avlist;
    pool->avlist = retval->next;
    return retval;
  }
  return (KlrItemSet*)malloc(sizeof (KlrItemSet));
}

static inline void klr_itemsetpool_deallocate(KlrItemSetPool* pool, KlrItemSet* itemset) {
  if (!itemset) return;
  itemset->next = pool->avlist;
  pool->avlist = itemset;
}


#endif
