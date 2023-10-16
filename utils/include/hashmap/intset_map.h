#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_INTSET_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_INTSET_MAP_H

#include "utils/include/set/bitset.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevIntSetMapNode {
  size_t key;
  KBitSet* value;
  struct tagKevIntSetMapNode* next;
} KevIntSetMapNode;

typedef struct tagKevIntSetMapBucket {
  KevIntSetMapNode* map_node_list;
  struct tagKevIntSetMapBucket* next;
} KevIntSetMapBucket;

typedef struct tagKevIntSetMap {
  KevIntSetMapBucket* array;
  KevIntSetMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevIntSetMap;

bool kev_intsetmap_init(KevIntSetMap* map, size_t capacity);
void kev_intsetmap_destroy(KevIntSetMap* map);

bool kev_intsetmap_insert(KevIntSetMap* map, size_t key, KBitSet* value);
KevIntSetMapNode* kev_intsetmap_search(KevIntSetMap* map, size_t key);
void kev_intsetmap_make_empty(KevIntSetMap* map);

static inline KevIntSetMapNode* kev_intsetmap_iterate_begin(KevIntSetMap* map);
KevIntSetMapNode* kev_intsetmap_iterate_next(KevIntSetMap* map, KevIntSetMapNode* current);

static inline KevIntSetMapNode* kev_intsetmap_iterate_begin(KevIntSetMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
