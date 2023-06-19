#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_INTSET_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_FINITE_AUTOMATA_HASHMAP_INTSET_MAP_H

#include "tokenizer_generator/include/finite_automata/bitset/bitset.h"
#include "tokenizer_generator/include/general/global_def.h"

typedef struct tagKevIntSetMapNode {
  uint64_t key;
  KevBitSet* value;
  struct tagKevIntSetMapNode* next;
} KevIntSetMapNode;

typedef struct tagKevIntSetMapBucket {
  KevIntSetMapNode* map_node_list;
  struct tagKevIntSetMapBucket* next;
} KevIntSetMapBucket;

typedef struct tagKevIntSetMap {
  KevIntSetMapBucket* array;
  KevIntSetMapBucket* bucket_head;
  uint64_t capacity;
  uint64_t size;
} KevIntSetMap;


bool kev_intset_map_init(KevIntSetMap* map, uint64_t capacity);
void kev_intset_map_destroy(KevIntSetMap* map);

bool kev_intset_map_insert(KevIntSetMap* map, uint64_t key, KevBitSet* value);
KevIntSetMapNode* kev_intset_map_search(KevIntSetMap* map, uint64_t key);
void kev_intset_map_make_empty(KevIntSetMap* map);

static inline KevIntSetMapNode* kev_intset_map_iterate_begin(KevIntSetMap* map);
KevIntSetMapNode* kev_intset_map_iterate_next(KevIntSetMap* map, KevIntSetMapNode* current);

static inline KevIntSetMapNode* kev_intset_map_iterate_begin(KevIntSetMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}


#endif
