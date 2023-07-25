#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_STRMAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_STRMAP_H

#include "utils/include/general/global_def.h"

typedef struct tagKevStringMapNode {
  char* key;
  char* value;
  struct tagKevStringMapNode* next;
} KevStringMapNode;

typedef struct tagKevStringMapBucket {
  KevStringMapNode* map_node_list;
  struct tagKevStringMapBucket* next;
} KevStringMapBucket;

typedef struct tagKevStringMap {
  KevStringMapBucket* array;
  KevStringMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevStringMap;

bool kev_strmap_init(KevStringMap* map, size_t capacity);
KevStringMap* kev_strmap_create(size_t capacity);
void kev_strmap_destroy(KevStringMap* map);
void kev_strmap_delete(KevStringMap* map);

bool kev_strmap_insert(KevStringMap* map, char* key, char* value);
bool kev_strmap_insert_move(KevStringMap* map, char* key, char* value);
KevStringMapNode* kev_strmap_search(KevStringMap* map, char* key);
bool kev_strmap_update(KevStringMap* map, char* key, char* value);
bool kev_strmap_update_move(KevStringMap* map, char* key, char* value);

static inline KevStringMapNode* kev_strmap_iterate_begin(KevStringMap* map);
KevStringMapNode* kev_strmap_iterate_next(KevStringMap* map, KevStringMapNode* current);

static inline KevStringMapNode* kev_strmap_iterate_begin(KevStringMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
