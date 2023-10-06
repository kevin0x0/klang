#ifndef KEVCC_PARGEN_INCLUDE_PARSER_SYMTABLE_H
#define KEVCC_PARGEN_INCLUDE_PARSER_SYMTABLE_H
#include "utils/include/hashmap/str_map.h"

typedef KevStringMapNode KevSymTableNode;
typedef struct tagSymTable {
  size_t parent;
  size_t self;
  KevStringMap table;
} KevSymTable;

KevSymTable* kev_symtable_create(size_t self, size_t parent);
void kev_symtable_delete(KevSymTable* symtable);

static inline bool kev_symtable_insert(KevSymTable* map, const char* key, const char* value);
static inline bool kev_symtable_insert_move(KevSymTable* map, const char* key, char* value);
static inline KevSymTableNode* kev_symtable_search(KevSymTable* map, const char* key);
static inline bool kev_symtable_update(KevSymTable* map, const char* key, const char* value);
static inline bool kev_symtable_update_move(KevSymTable* map, const char* key, char* value);

static inline const char* kev_symtable_node_get_key(KevSymTableNode* node);
static inline const char* kev_symtable_node_get_value(KevSymTableNode* node);
static inline char* kev_symtable_node_swap_value(KevSymTableNode* mapnode, char* value);

static inline bool kev_symtable_node_set_value(KevSymTableNode* node, const char* value);
static inline void kev_symtable_node_set_value_move(KevSymTableNode* node, char* value);

static inline KevSymTableNode* kev_symtable_iterate_begin(KevSymTable* map);
static inline KevSymTableNode* kev_symtable_iterate_next(KevSymTable* map, KevSymTableNode* current);


static inline bool kev_symtable_insert(KevSymTable* map, const char* key, const char* value) {
  return kev_strmap_insert(&map->table, key, value);
}

static inline bool kev_symtable_insert_move(KevSymTable* map, const char* key, char* value) {
  return kev_strmap_insert_move(&map->table, key, value);
}

static inline KevSymTableNode* kev_symtable_search(KevSymTable* map, const char* key) {
  return kev_strmap_search(&map->table, key);
}

static inline bool kev_symtable_update(KevSymTable* map, const char* key, const char* value) {
  return kev_strmap_update(&map->table, key, value);
}

static inline bool kev_symtable_update_move(KevSymTable* map, const char* key, char* value) {
  return kev_strmap_update_move(&map->table, key, value);
}

static inline const char* kev_symtable_node_get_key(KevSymTableNode* node) {
  return kev_strmap_node_get_key(node);
}

static inline const char* kev_symtable_node_get_value(KevSymTableNode* node) {
  return kev_strmap_node_get_value(node);
}

static inline char* kev_symtable_node_swap_value(KevSymTableNode* node, char* value) {
  return kev_strmap_node_swap_value(node, value);
}

static inline bool kev_symtable_node_set_value(KevSymTableNode* node, const char* value) {
  return kev_strmap_node_set_value(node, value);
}

static inline void kev_symtable_node_set_value_move(KevSymTableNode* node, char* value) {
  return kev_strmap_node_set_value_move(node, value);
}

static inline KevSymTableNode* kev_symtable_iterate_begin(KevSymTable* map) {
  return kev_strmap_iterate_begin(&map->table);
}

static inline KevSymTableNode* kev_symtable_iterate_next(KevSymTable* map, KevSymTableNode* current) {
  return kev_strmap_iterate_next(&map->table, current);
}

#endif
