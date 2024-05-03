#ifndef KEVCC_KLANG_INCLUDE_KLMAP_H
#define KEVCC_KLANG_INCLUDE_KLMAP_H

#include "include/mm/klmm.h"
#include "include/value/klclass.h"
#include "include/value/klvalue.h"
#include "include/value/klstring.h"

#include <stddef.h>
#include <stdbool.h>

#define KLMAP_OPT_WEAKKEY   (klbit(0))
#define KLMAP_OPT_WEAKVAL   (klbit(1))

typedef struct tagKlMapNode KlMapNode;
typedef KlMapNode* KlMapIter;
typedef struct tagKlMapNodePool KlMapNodePool;

struct tagKlMapNode {
  KlMapNode* next;
  KlMapNode* prev;
  KlValue key;
  KlValue value;
  size_t hash;
};

typedef struct tagKlMap {
  KL_DERIVE_FROM(KlObject, _objectbase_);
  KlMapNode** array;
  KlMapNode head;
  KlMapNode tail;
  KlMapNodePool* nodepool;
  size_t capacity;
  size_t size;
  unsigned option;
  KLOBJECT_TAIL;
} KlMap;


KlMap* klmap_create(KlClass* mapclass, size_t capacity, KlMapNodePool* nodepool);
KlClass* klmap_class(KlMM* klmm, KlMapNodePool* mapnodepool);

static inline size_t klmap_size(KlMap* map);
static inline size_t klmap_capacity(KlMap* map);

KlMapIter klmap_insert(KlMap* map, KlValue* key, KlValue* value);
KlMapIter klmap_erase(KlMap* map, KlMapIter iter);
KlMapIter klmap_search(KlMap* map, KlValue* key);
KlMapIter klmap_searchstring(KlMap* map, KlString* str);
KlMapIter klmap_insertstring(KlMap* map, KlString* str, KlValue* val);
void klmap_makeempty(KlMap* map);
static inline void klmap_index(KlMap* map, KlValue* key, KlValue* val);
static inline bool klmap_indexas(KlMap* map, KlValue* key, KlValue* val);

static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* node);

static inline KlMapIter klmap_iter_begin(KlMap* map);
static inline KlMapIter klmap_iter_end(KlMap* map);
static inline KlMapIter klmap_iter_next(KlMapIter current);

static inline void klmap_index(KlMap* map, KlValue* key, KlValue* val) {
  KlMapIter iter = klmap_search(map, key);
  iter ? klvalue_setnil(val) : klvalue_setvalue(val, &iter->value);
}

static inline bool klmap_indexas(KlMap* map, KlValue* key, KlValue* val) {
  KlMapIter iter = klmap_search(map, key);
  if (iter) {
    if (kl_unlikely(!klmap_insert(map, key, val)))
      return false;
  } else {
    klvalue_setvalue(&iter->value, val);
  }
  return true;
}

static inline KlMapIter klmap_iter_begin(KlMap* map) {
    return map->head.next;
}

static inline KlMapIter klmap_iter_end(KlMap* map) {
  return &map->tail;
}

static inline size_t klmap_size(KlMap* map) {
  return map->size;
}

static inline size_t klmap_capacity(KlMap* map) {
  return map->capacity;
}

static inline KlMapIter klmap_iter_next(KlMapIter current) {
  return current->next;
}




struct tagKlMapNodePool {
  KlMM* klmm;
  size_t ref_count;
  KlMapNode* nodes;
  size_t available;
};


KlMapNodePool* klmapnodepool_create(KlMM* klmm);
static inline void klmapnodepool_pin(KlMapNodePool* nodepool);
static inline void klmapnodepool_unpin(KlMapNodePool* nodepool);
void klmapnodepool_delete(KlMapNodePool* nodepool);

static inline KlMM* klmapnodepool_getmm(KlMapNodePool* nodepool);

static inline KlMapNode* klmapnodepool_alloc(KlMapNodePool* nodepool);
static inline void klmapnodepool_free(KlMapNodePool* nodepool, KlMapNode* node);
static inline void klmapnodepool_freelist(KlMapNodePool* nodepool, KlMapNode* head, KlMapNode* tail, size_t count);
void klmapnodepool_shrink(KlMapNodePool* nodepool);

static inline void klmapnodepool_unpin(KlMapNodePool* nodepool) {
  if (--nodepool->ref_count)
    return;
  klmapnodepool_delete(nodepool);
}

static inline void klmapnodepool_pin(KlMapNodePool* nodepool) {
  ++nodepool->ref_count;
}

static inline KlMM* klmapnodepool_getmm(KlMapNodePool* nodepool) {
  return nodepool->klmm;
}

static inline KlMapNode* klmapnodepool_alloc(KlMapNodePool* nodepool) {
  KlMapNode* node = nodepool->nodes;
  if (!node)
    return (KlMapNode*)klmm_alloc(nodepool->klmm, sizeof (KlMapNode));
  nodepool->nodes = node->next;
  --nodepool->available;
  return node;
}

static inline void klmapnodepool_free(KlMapNodePool* nodepool, KlMapNode* node) {
  node->next = nodepool->nodes;
  nodepool->nodes = node;
  ++nodepool->available;
}

static inline void klmapnodepool_freelist(KlMapNodePool* nodepool, KlMapNode* head, KlMapNode* tail, size_t count) {
  tail->next = nodepool->nodes;
  nodepool->nodes = head;
  nodepool->available += count;
}





static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* node) {
  node->prev = insertpos->prev;
  insertpos->prev->next = node;
  node->next = insertpos;
  insertpos->prev = node;
}

#endif
