#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_HASHMAP_STRNFA_MAP_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_HASHMAP_STRNFA_MAP_H

#include "kevfa/include/finite_automaton.h"

typedef struct tagKevStringFaMapNode {
  char* key;
  KevFA* value;
  size_t hashval;
  struct tagKevStringFaMapNode* next;
} KevStringFaMapNode;

typedef struct tagKevStringFaMapBucket {
  KevStringFaMapNode* map_node_list;
  struct tagKevStringFaMapBucket* next;
} KevStringFaMapBucket;

typedef struct tagKevStringFaMap {
  KevStringFaMapBucket* array;
  KevStringFaMapBucket* bucket_head;
  size_t capacity;
  size_t size;
} KevStringFaMap;

bool kev_strfamap_init(KevStringFaMap* map, size_t capacity);
KevStringFaMap* kev_strfamap_create(size_t capacity);
void kev_strfamap_destroy(KevStringFaMap* map);
void kev_strfamap_delete(KevStringFaMap* map);

bool kev_strfamap_insert(KevStringFaMap* map, char* key, KevFA* value);
KevStringFaMapNode* kev_strfamap_search(KevStringFaMap* map, char* key);
bool kev_strfamap_update(KevStringFaMap* map, char* key, KevFA* value);
void kev_strfamap_make_empty(KevStringFaMap* map);

static inline KevStringFaMapNode* kev_strfamap_iterate_begin(KevStringFaMap* map);
KevStringFaMapNode* kev_strfamap_iterate_next(KevStringFaMap* map, KevStringFaMapNode* current);

static inline KevStringFaMapNode* kev_strfamap_iterate_begin(KevStringFaMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}

#endif
