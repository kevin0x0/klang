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

static inline bool kev_symtable_insert(KevSymTable* symtable, const char* key, const char* value);
static inline bool kev_symtable_insert_move(KevSymTable* symtable, const char* key, char* value);
static inline KevSymTableNode* kev_symtable_search(KevSymTable* symtable, const char* key);
static inline bool kev_symtable_update(KevSymTable* symtable, const char* key, const char* value);
static inline bool kev_symtable_update_move(KevSymTable* symtable, const char* key, char* value);
static inline size_t kev_symtable_get_self_id(KevSymTable* symtable);
static inline size_t kev_symtable_get_parent_id(KevSymTable* symtable);

static inline const char* kev_symtable_node_get_key(KevSymTableNode* node);
static inline const char* kev_symtable_node_get_value(KevSymTableNode* node);
static inline char* kev_symtable_node_swap_value(KevSymTableNode* mapnode, char* value);

static inline bool kev_symtable_node_set_value(KevSymTableNode* node, const char* value);
static inline void kev_symtable_node_set_value_move(KevSymTableNode* node, char* value);

static inline KevSymTableNode* kev_symtable_iterate_begin(KevSymTable* symtable);
static inline KevSymTableNode* kev_symtable_iterate_next(KevSymTable* symtable, KevSymTableNode* current);


static inline bool kev_symtable_insert(KevSymTable* symtable, const char* key, const char* value) {
  return kev_strmap_insert(&symtable->table, key, value);
}

static inline bool kev_symtable_insert_move(KevSymTable* symtable, const char* key, char* value) {
  return kev_strmap_insert_move(&symtable->table, key, value);
}

static inline KevSymTableNode* kev_symtable_search(KevSymTable* symtable, const char* key) {
  return kev_strmap_search(&symtable->table, key);
}

static inline bool kev_symtable_update(KevSymTable* symtable, const char* key, const char* value) {
  return kev_strmap_update(&symtable->table, key, value);
}

static inline bool kev_symtable_update_move(KevSymTable* symtable, const char* key, char* value) {
  return kev_strmap_update_move(&symtable->table, key, value);
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

static inline KevSymTableNode* kev_symtable_iterate_begin(KevSymTable* symtable) {
  return kev_strmap_iterate_begin(&symtable->table);
}

static inline KevSymTableNode* kev_symtable_iterate_next(KevSymTable* symtable, KevSymTableNode* current) {
  return kev_strmap_iterate_next(&symtable->table, current);
}

static inline size_t kev_symtable_get_self_id(KevSymTable* symtable) {
  return symtable->self;
}

static inline size_t kev_symtable_get_parent_id(KevSymTable* symtable) {
  return symtable->parent;
}

#endif
