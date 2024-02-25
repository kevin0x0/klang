#ifndef KEVCC_KLANG_INCLUDE_KLMAP_H
#define KEVCC_KLANG_INCLUDE_KLMAP_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klclass.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/value/klstring.h"

#include <stddef.h>
#include <stdbool.h>


typedef struct tagKlMapNode KlMapNode;
typedef KlMapNode* KlMapIter;
typedef struct tagKlMapNodePool KlMapNodePool;

struct tagKlMapNode {
  KlValue key;
  KlValue value;
  size_t hash;
  KlMapNode* next;
  KlMapNode* prev;
};

typedef struct tagKlMap {
  KlObject objbase;
  KlMapNode** array;
  KlMapNode head;
  KlMapNode tail;
  KlMapNodePool* nodepool;
  size_t capacity;
  size_t size;
  klobject_tail;
} KlMap;


KlMap* klmap_create(KlClass* mapclass, size_t capacity, KlMapNodePool* nodepool);
KlClass* klmap_class(KlMM* klmm, KlMapNodePool* mapnodepool);

static inline size_t klmap_size(KlMap* map);
static inline size_t klmap_capacity(KlMap* map);


KlMapIter klmap_insert(KlMap* map, KlValue* key, KlValue* value);
KlMapIter klmap_erase(KlMap* map, KlMapIter iter);
KlMapIter klmap_search(KlMap* map, KlValue* key);
static inline void klmap_index(KlMap* map, KlValue* key, KlValue* val);
static inline bool klmap_indexas(KlMap* map, KlValue* key, KlValue* val);


static inline KlMapIter klmap_iter_begin(KlMap* map);
static inline KlMapIter klmap_iter_end(KlMap* map);
static inline KlMapIter klmap_iter_next(KlMapIter current);


static inline void klmap_index(KlMap* map, KlValue* key, KlValue* val) {
  KlMapIter iter = klmap_search(map, key);
  if (iter == klmap_iter_end(map)) {
    klvalue_setnil(val);
  } else {
    klvalue_setvalue(val, &iter->value);
  }
}

static inline bool klmap_indexas(KlMap* map, KlValue* key, KlValue* val) {
  KlMapIter iter = klmap_search(map, key);
  if (iter == klmap_iter_end(map)) {
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


#endif
