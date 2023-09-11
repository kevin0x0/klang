#ifndef KEVCC_UTILS_INCLUDE_HASHMAP_STRX_MAP_H
#define KEVCC_UTILS_INCLUDE_HASHMAP_STRX_MAP_H

#include "utils/include/general/global_def.h"

typedef struct tagKevStrXMapNode {
  char* key;
  void* value;
  size_t hashval;
  struct tagKevStrXMapNode* next;
} KevStrXMapNode;

typedef struct tagKevStrXMapBucket {
  KevStrXMapNode* map_node_list;
  struct tagKevStrXMapBucket* next;
} KevStrXMapBucket;

typedef struct tagKevStrXMap {
  KevStrXMapBucket* array;
  KevStrXMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevStrXMap;

bool kev_strxmap_init(KevStrXMap* map, size_t capacity);
KevStrXMap* kev_strxmap_create(size_t capacity);
void kev_strxmap_destroy(KevStrXMap* map);
void kev_strxmap_delete(KevStrXMap* map);

bool kev_strxmap_insert(KevStrXMap* map, const char* key, void* value);
KevStrXMapNode* kev_strxmap_search(KevStrXMap* map, const char* key);

static inline KevStrXMapNode* kev_strxmap_iterate_begin(KevStrXMap* map);
KevStrXMapNode* kev_strxmap_iterate_next(KevStrXMap* map, KevStrXMapNode* current);

static inline KevStrXMapNode* kev_strxmap_iterate_begin(KevStrXMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
