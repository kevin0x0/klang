#ifndef KEVCC_KEVLR_INCLUDE_HASHMAP_PRIORITY_MAP_H
#define KEVCC_KEVLR_INCLUDE_HASHMAP_PRIORITY_MAP_H

#include "kevlr/include/rule.h"
#include "utils/include/general/global_def.h"

#define KLR_PRIOPOS_POSTFIX   (-1)

typedef int KlrPrioPos;

typedef struct tagKlrPrioMapNode {
  KlrSymbol* symbol;
  size_t priority;
  struct tagKlrPrioMapNode* next;
  KlrPrioPos pos;
} KlrPrioMapNode;

typedef struct tagKlrPrioMapBucket {
  KlrPrioMapNode* map_node_list;
  struct tagKlrPrioMapBucket* next;
} KlrPrioMapBucket;

typedef struct tagKlrPrioMap {
  KlrPrioMapBucket* array;
  KlrPrioMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KlrPrioMap;

bool klr_priomap_init(KlrPrioMap* map, size_t capacity);
KlrPrioMap* klr_priomap_create(size_t capacity);
void klr_priomap_destroy(KlrPrioMap* map);
void klr_priomap_delete(KlrPrioMap* map);

bool klr_priomap_insert(KlrPrioMap* map, KlrSymbol* symbol, KlrPrioPos pos, size_t priority);
KlrPrioMapNode* klr_priomap_search(KlrPrioMap* map, KlrSymbol* symbol, KlrPrioPos pos);
bool klr_priomap_update(KlrPrioMap* map, KlrSymbol* symbol, KlrPrioPos pos, size_t priority);
void klr_priomap_make_empty(KlrPrioMap* map);

static inline KlrPrioMapNode* klr_priomap_iterate_begin(KlrPrioMap* map);
KlrPrioMapNode* klr_priomap_iterate_next(KlrPrioMap* map, KlrPrioMapNode* current);

static inline KlrPrioMapNode* klr_priomap_iterate_begin(KlrPrioMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
