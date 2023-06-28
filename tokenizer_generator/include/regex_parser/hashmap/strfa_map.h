#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_REGEX_PARSER_HASHMAP_STRNFA_MAP_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_REGEX_PARSER_HASHMAP_STRNFA_MAP_H
#include "tokenizer_generator/include/finite_automaton/finite_automaton.h"

typedef struct tagKevStringFaMapNode {
  char* key;
  KevFA* value;
  struct tagKevStringFaMapNode* next;
} KevStringFaMapNode;

typedef struct tagKevStringFaMapBucket {
  KevStringFaMapNode* map_node_list;
  struct tagKevStringFaMapBucket* next;
} KevStringFaMapBucket;

typedef struct tagKevStringFaMap {
  KevStringFaMapBucket* array;
  KevStringFaMapBucket* bucket_head;
  uint64_t capacity;
  uint64_t size;
} KevStringFaMap;


bool kev_strfamap_init(KevStringFaMap* map, uint64_t capacity);
KevStringFaMap* kev_strfamap_create(uint64_t capacity);
void kev_strfamap_destroy(KevStringFaMap* map);
void kev_strfamap_delete(KevStringFaMap* map);

bool kev_strfamap_insert(KevStringFaMap* map, char* key, KevFA* value);
KevStringFaMapNode* kev_strfamap_search(KevStringFaMap* map, char* key);
void kev_strfamap_make_empty(KevStringFaMap* map);

static inline KevStringFaMapNode* kev_strfamap_iterate_begin(KevStringFaMap* map);
KevStringFaMapNode* kev_strfamap_iterate_next(KevStringFaMap* map, KevStringFaMapNode* current);

static inline KevStringFaMapNode* kev_strfamap_iterate_begin(KevStringFaMap* map) {
    return map->bucket_head ? map->bucket_head->map_node_list : NULL;
}
#endif
