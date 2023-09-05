#ifndef KEVCC_KEVFA_INCLUDE_HASHMAP_INTLIST_MAP_H
#define KEVCC_KEVFA_INCLUDE_HASHMAP_INTLIST_MAP_H

#include "kevfa/include/list/node_list.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevIntListMapNode {
  size_t key;
  KevNodeList* value;
  struct tagKevIntListMapNode* next;
} KevIntListMapNode;

typedef struct tagKevIntListMapBucket {
  KevIntListMapNode* map_node_list;
  struct tagKevIntListMapBucket* next;
} KevIntListMapBucket;

typedef struct tagKevIntListMap {
  KevIntListMapBucket* array;
  KevIntListMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevIntListMap;

bool kev_intlistmap_init(KevIntListMap* map, size_t capacity);
void kev_intlistmap_destroy(KevIntListMap* map);

KevIntListMapNode* kev_intlistmap_insert(KevIntListMap* map, size_t key, KevNodeList* value);
KevIntListMapNode* kev_intlistmap_search(KevIntListMap* map, size_t key);
void kev_intlistmap_make_empty(KevIntListMap* map);

static inline KevIntListMapNode* kev_intlistmap_iterate_begin(KevIntListMap* map);
KevIntListMapNode* kev_intlistmap_iterate_next(KevIntListMap* map, KevIntListMapNode* current);

static inline KevIntListMapNode* kev_intlistmap_iterate_begin(KevIntListMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
