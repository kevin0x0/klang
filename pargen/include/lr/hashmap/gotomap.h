#ifndef KEVCC_PARGEN_INCLUDE_HASHMAP_GOTOMAP_H
#define KEVCC_PARGEN_INCLUDE_HASHMAP_GOTOMAP_H

#include "pargen/include/lr/lalr.h"
#include "utils/include/general/global_def.h"

typedef struct tagKevGotoMapNode {
  KevSymbol* key;
  KevItemSet* value;
  struct tagKevGotoMapNode* next;
} KevGotoMapNode;

typedef struct tagKevGotoMapBucket {
  KevGotoMapNode* map_node_list;
  struct tagKevGotoMapBucket* next;
} KevGotoMapBucket;

typedef struct tagKevGotoMap {
  KevGotoMapBucket* array;
  KevGotoMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevGotoMap;

bool kev_gotomap_init(KevGotoMap* map, size_t capacity);
KevGotoMap* kev_gotomap_create(size_t capacity);
void kev_gotomap_destroy(KevGotoMap* map);
void kev_gotomap_delete(KevGotoMap* map);

bool kev_gotomap_insert(KevGotoMap* map, KevSymbol* key, KevItemSet* value);
bool kev_gotomap_insert_move(KevGotoMap* map, KevSymbol* key, KevItemSet* value);
KevGotoMapNode* kev_gotomap_search(KevGotoMap* map, KevSymbol* key);
bool kev_gotomap_update(KevGotoMap* map, KevSymbol* key, KevItemSet* value);
bool kev_gotomap_update_move(KevGotoMap* map, KevSymbol* key, KevItemSet* value);
void kev_gotomap_make_empty(KevGotoMap* map);

static inline KevGotoMapNode* kev_gotomap_iterate_begin(KevGotoMap* map);
KevGotoMapNode* kev_gotomap_iterate_next(KevGotoMap* map, KevGotoMapNode* current);

static inline KevGotoMapNode* kev_gotomap_iterate_begin(KevGotoMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
