#ifndef KEVCC_KEVLR_INCLUDE_HASHMAP_TRANS_MAP_H
#define KEVCC_KEVLR_INCLUDE_HASHMAP_TRANS_MAP_H

#include "kevlr/include/collection.h"
#include "utils/include/general/global_def.h"

typedef struct tagKlrTransMapNode {
  KlrSymbol* key;
  KlrItemSet* value;
  struct tagKlrTransMapNode* next;
} KlrTransMapNode;

typedef struct tagKlrTransMapBucket {
  KlrTransMapNode* map_node_list;
  struct tagKlrTransMapBucket* next;
} KlrTransMapBucket;

typedef struct tagKlrTransMap {
  KlrTransMapBucket* array;
  KlrTransMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KlrTransMap;

bool klr_transmap_init(KlrTransMap* map, size_t capacity);
KlrTransMap* klr_transmap_create(size_t capacity);
void klr_transmap_destroy(KlrTransMap* map);
void klr_transmap_delete(KlrTransMap* map);

bool klr_transmap_insert(KlrTransMap* map, KlrSymbol* key, KlrItemSet* value);
KlrTransMapNode* klr_transmap_search(KlrTransMap* map, KlrSymbol* key);
bool klr_transmap_update(KlrTransMap* map, KlrSymbol* key, KlrItemSet* value);
void klr_transmap_make_empty(KlrTransMap* map);

static inline KlrTransMapNode* klr_transmap_iter_begin(KlrTransMap* map);
KlrTransMapNode* klr_transmap_iter_next(KlrTransMap* map, KlrTransMapNode* current);

static inline KlrTransMapNode* klr_transmap_iter_begin(KlrTransMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
