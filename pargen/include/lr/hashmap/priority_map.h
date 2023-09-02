#ifndef KEVCC_PARGEN_INCLUDE_HASHMAP_PRIORITY_MAP_H
#define KEVCC_PARGEN_INCLUDE_HASHMAP_PRIORITY_MAP_H

#include "pargen/include/lr/rule.h"
#include "utils/include/general/global_def.h"

#define KEV_LR_SYMBOL_POS_POSTFIX (-1)

typedef int KevPrioPos;

typedef struct tagKevPrioMapNode {
  KevSymbol* symbol;
  size_t priority;
  struct tagKevPrioMapNode* next;
  KevPrioPos pos;
} KevPrioMapNode;

typedef struct tagKevPrioMapBucket {
  KevPrioMapNode* map_node_list;
  struct tagKevPrioMapBucket* next;
} KevPrioMapBucket;

typedef struct tagKevPrioMap {
  KevPrioMapBucket* array;
  KevPrioMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevPrioMap;

bool kev_priomap_init(KevPrioMap* map, size_t capacity);
KevPrioMap* kev_priomap_create(size_t capacity);
void kev_priomap_destroy(KevPrioMap* map);
void kev_priomap_delete(KevPrioMap* map);

bool kev_priomap_insert(KevPrioMap* map, KevSymbol* symbol, KevPrioPos pos, size_t priority);
KevPrioMapNode* kev_priomap_search(KevPrioMap* map, KevSymbol* symbol, KevPrioPos pos);
bool kev_priomap_update(KevPrioMap* map, KevSymbol* symbol, KevPrioPos pos, size_t priority);
void kev_priomap_make_empty(KevPrioMap* map);

static inline KevPrioMapNode* kev_priomap_iterate_begin(KevPrioMap* map);
KevPrioMapNode* kev_priomap_iterate_next(KevPrioMap* map, KevPrioMapNode* current);

static inline KevPrioMapNode* kev_priomap_iterate_begin(KevPrioMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
