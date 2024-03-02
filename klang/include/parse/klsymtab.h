#ifndef KEVCC_KLANG_INCLUDE_KLSYMTAB_H
#define KEVCC_KLANG_INCLUDE_KLSYMTAB_H

//#include "utils/include/kstring/kstring.h"
//
//#include <stddef.h>
//#include <stdbool.h>
//
//typedef struct tagKlSymAttr {
//  KlValPos valpos;
//} KlSymAttr;
//
//typedef struct tagKlSymTabNode {
//  KString* name;
//  KlSymAttr attr;
//  size_t hashval;
//  struct tagKlSymTabNode* next;
//  struct tagKlSymTabNode* prev;
//} KlSymTabNode;
//
//typedef KlSymTabNode* KlSymTabIter;
//
//typedef enum tagKlScope { KLSCP_FUNC, KLSCP_INHERIT, KLSCP_OTHER } KlScope;
//
//typedef struct tagKlSymTab {
//  KlSymTabNode** array;
//  KlSymTabNode* head;
//  KlSymTabNode* tail;
//  size_t capacity;
//  size_t size;
//  /* symbol table attribute */
//  struct tagKlSymTab* parent;
//  KlScope scope;
//} KlSymTab;
//
//bool klsymtab_init(KlSymTab* map, KlSymTab* parent, KlScope scope);
//KlSymTab* klsymtab_create(KlSymTab* parent, KlScope scope);
//void klsymtab_destroy(KlSymTab* map);
//void klsymtab_delete(KlSymTab* map);
//
//static inline KlSymTab* klsymtab_get_parent(KlSymTab* map);
//static inline KlScope klsymtab_get_scope(KlSymTab* map);
//
//static inline void klsymtab_set_scope(KlSymTab* map, KlScope scope);
//
//static inline size_t klsymtab_size(KlSymTab* map);
//static inline size_t klsymtab_capacity(KlSymTab* map);
//
//static inline KlSymTabIter klsymtab_insert(KlSymTab* map, const KString* key);
//KlSymTabIter  klsymtab_insert_move(KlSymTab* map, KString* key);
//KlSymTabIter klsymtab_erase(KlSymTab* map, KlSymTabIter iter);
//KlSymTabIter klsymtab_search(KlSymTab* map, const KString* key);
//
//static inline KlSymTabIter klsymtab_iter_begin(KlSymTab* map);
//static inline KlSymTabIter klsymtab_iter_end(KlSymTab* map);
//static inline KlSymTabIter klsymtab_iter_next(KlSymTabIter current);
//
//
//static inline KlSymTab* klsymtab_get_parent(KlSymTab* map) {
//  return map->parent;
//}
//
//static inline KlScope klsymtab_get_scope(KlSymTab* map) {
//  return map->scope;
//}
//
//static inline void klsymtab_set_scope(KlSymTab* map, KlScope scope) {
//  map->scope = scope;
//}
//
//static inline KlSymTabIter klsymtab_iter_begin(KlSymTab* map) {
//    return map->head->next;
//}
//
//static inline KlSymTabIter klsymtab_iter_end(KlSymTab* map) {
//  return map->tail;
//}
//
//static inline size_t klsymtab_size(KlSymTab* map) {
//  return map->size;
//}
//
//static inline size_t klsymtab_capacity(KlSymTab* map) {
//  return map->capacity;
//}
//
//static inline KlSymTabIter klsymtab_iter_next(KlSymTabIter current) {
//  return current->next;
//}
//
//static inline KlSymTabIter klsymtab_insert(KlSymTab* map, const KString* key) {
//  KString* copy = kstring_create_copy(key);
//  KlSymTabIter itr = NULL;
//  if (!copy || !(itr = klsymtab_insert_move(map, copy))) {
//    kstring_delete(copy);
//  }
//  return itr;
//}
//
#endif
