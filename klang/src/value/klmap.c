#include "klang/include/value/klmap.h"
#include "klang/include/mm/klmm.h"
#include "klang/include/value/klvalue.h"

#include <stdlib.h>
#include <string.h>

static KlGCObject* klmap_propagate(KlMap* map, KlGCObject* gclist);
static void klmap_delete(KlMap* map);

static KlGCVirtualFunc klmap_gcvfunc = { .propagate = (KlGCProp)klmap_propagate, .destructor = (KlGCDestructor)klmap_delete };


static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* elem);
static inline void klmap_init_head_tail(KlMapNode* head, KlMapNode* tail);

static inline size_t klmap_gethash(KlValue* key) {
  if (klvalue_checktype(key, KL_STRING)) {
    return klstring_hash(klvalue_getobj(key, KlString*));
  } else if (klvalue_checktype(key, KL_INT) || klvalue_checktype(key, KL_BOOL)) {
    return klvalue_getint(key);
  } else if (klvalue_checktype(key, KL_FLOAT)) {
    kl_assert(sizeof (KlFloat) == sizeof (KlInt), "");
    struct {
      size_t hash;
      KlFloat floatval;
    } num;
    num.floatval = klvalue_getfloat(key);
    /* +0.0 and -0.0 is equal but have difference binary representations */
    if (num.floatval == 0.0) return 0;
    return num.hash;
  } else {
    return ((size_t)klvalue_getany(key) >> 3) + klvalue_gettype(key);
  }
}

static inline void klmap_init_head_tail(KlMapNode* head, KlMapNode* tail) {
  head->next = tail;
  tail->prev = head;
  head->prev = NULL;
  tail->next = NULL;
}

static void klmap_rehash(KlMap* map, KlMapNode** new_array, size_t new_capacity) {
  size_t mask = new_capacity - 1;
  KlMapNode* node = map->head.next;
  KlMapNode* end = &map->tail;

  KlMapNode tmphead;
  KlMapNode tmptail;
  klmap_init_head_tail(&tmphead, &tmptail);

  while (node != end) {
    KlMapNode* tmp = node->next;
    size_t index = node->hash & mask;
    if (!new_array[index]) {
      /* this means 'node' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
      new_array[index] = node;
      klmap_node_insert(tmphead.next, node);
    } else {
      klmap_node_insert(new_array[index]->next, node);
    }
    node = tmp;
  }

  tmphead.next->prev = &map->head;
  map->head.next = tmphead.next;
  tmptail.prev->next = &map->tail;
  map->tail.prev = tmptail.prev;

  klmm_free(klmm_gcobj_getmm(klmm_to_gcobj(map)), map->array, map->capacity * sizeof (KlMapNode*));
  map->array = new_array;
  map->capacity = new_capacity;
}

static inline bool klmap_expand(KlMap* map) {
  size_t new_capacity = map->capacity << 1;
  KlMapNode** new_array = klmm_alloc(klmm_gcobj_getmm(klmm_to_gcobj(map)), new_capacity * sizeof (KlMapNode*));
  if (kl_unlikely(!new_array)) return false;
  for (size_t i = 0; i < new_capacity; ++i)
    new_array[i] = NULL;
  klmap_rehash(map, new_array, new_capacity);
  return true;
}

KlMap* klmap_create(KlClass* mapclass, size_t capacity, KlMapNodePool* nodepool) {
  capacity = ((size_t)1) << capacity;

  KlMM* klmm = klmm_gcobj_getmm(klmm_to_gcobj(mapclass));
  KlMap* map = (KlMap*)klclass_objalloc(mapclass, klmm);
  if (kl_unlikely(!map)) return NULL;

  KlMapNode** array = (KlMapNode**)klmm_alloc(klmm, sizeof (KlMapNode*) * capacity);
  if (kl_unlikely(!array)) {
    klobject_free(klcast(KlObject*, map), klmm);
    return NULL;
  }
  for (size_t i = 0; i < capacity; ++i)
    array[i] = NULL;
  klmap_init_head_tail(&map->head, &map->tail);
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  map->nodepool = nodepool;
  klmapnodepool_pin(nodepool);
  klmm_gcobj_enable(klmm, klmm_to_gcobj(map), &klmap_gcvfunc);
  return map;
}

static void klmap_delete(KlMap* map) {
  if (klmap_size(map) != 0)
    klmapnodepool_freelist(map->nodepool, map->head.next, map->tail.prev, klmap_size(map));

  klmapnodepool_unpin(map->nodepool);
  KlMM* klmm = klmm_gcobj_getmm(klmm_to_gcobj(map));
  klmm_free(klmm, map->array, map->capacity * sizeof (KlMapNode*));
  klobject_free(klcast(KlObject*, map), klmm);
}

KlMapIter klmap_insert(KlMap* map, KlValue* key, KlValue* value) {
  if (kl_unlikely(map->size >= map->capacity && !klmap_expand(map)))
    return NULL;

  KlMapNode* new_node = klmapnodepool_alloc(map->nodepool);
  if (kl_unlikely(!new_node)) return NULL;

  size_t hash = klmap_gethash(key);
  size_t index = (map->capacity - 1) & hash;
  new_node->hash = hash;
  klvalue_setvalue(&new_node->key, key);
  klvalue_setvalue(&new_node->value, value);
  if (!map->array[index]) {
    map->array[index] = new_node;
    klmap_node_insert(map->head.next, new_node);
  } else {
    klmap_node_insert(map->array[index]->next, new_node);
  }
  map->size++;
  return new_node;
}

KlMapIter klmap_search(KlMap* map, KlValue* key) {
  size_t mask = map->capacity - 1;
  size_t index = mask & klmap_gethash(key);
  KlMapNode* node = map->array[index];
  if (!node) return NULL;
  do {
    if (klvalue_equal(key, &node->key))
      return node;
    node = node->next;
  } while (node != &map->tail && (node->hash & mask) == index);
  return NULL;
}

KlMapIter klmap_searchstring(KlMap* map, KlString* str) {
  size_t mask = map->capacity - 1;
  size_t index = mask & klstring_hash(str);
  KlMapNode* node = map->array[index];
  if (!node) return NULL;
  do {
    if (str == klvalue_getobj(&node->key, KlString*) &&
        klvalue_checktype(&node->key, KL_STRING))
      return node;
    node = node->next;
  } while (node != &map->tail && (node->hash & mask) == index);
  return NULL;
}

KlMapIter klmap_insertstring(KlMap* map, KlString* str, KlValue* val) {
  if (kl_unlikely(map->size >= map->capacity && !klmap_expand(map)))
    return NULL;

  KlMapNode* new_node = klmapnodepool_alloc(map->nodepool);
  if (kl_unlikely(!new_node)) return NULL;

  size_t hash = klstring_hash(str);
  size_t index = (map->capacity - 1) & hash;
  new_node->hash = hash;
  klvalue_setobj(&new_node->key, str, KL_STRING);
  klvalue_setvalue(&new_node->value, val);
  if (!map->array[index]) {
    map->array[index] = new_node;
    klmap_node_insert(map->head.next, new_node);
  } else {
    klmap_node_insert(map->array[index]->next, new_node);
  }
  map->size++;
  return new_node;
}

KlMapIter klmap_erase(KlMap* map, KlMapIter iter) {
  iter->prev->next = iter->next;
  iter->next->prev = iter->prev;
  --map->size;
  size_t mask = (map->capacity - 1);
  size_t index = mask & iter->hash;
  KlMapIter next = iter->next;
  if (map->array[index] == iter) {
    if (next == &map->tail || (next->hash & mask) != index) {
      map->array[index] = NULL;
    } else {
      map->array[index] = next;
    }
  }
  klmapnodepool_free(map->nodepool, iter);
  return next;
}

static KlMap* klmap_constructor(KlClass* klclass) {
  return klmap_create(klclass, 3, klcast(KlMapNodePool*, klclass_constructor_data(klclass)));
}

KlClass* klmap_class(KlMM* klmm, KlMapNodePool* mapnodepool) {
  KlClass* mapclass = klclass_create(klmm, 32, klobject_attrarrayoffset(KlMap), mapnodepool, (KlObjectConstructor)klmap_constructor);
  return mapclass;
}

static KlGCObject* klmap_propagate(KlMap* map, KlGCObject* gclist) {
  KlMapIter end = klmap_iter_end(map);
  KlMapIter begin = klmap_iter_begin(map);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(&itr->key), gclist);
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(&itr->value), gclist);
  }
  return klobject_propagate(klcast(KlObject*, map), gclist);
}





KlMapNodePool* klmapnodepool_create(KlMM* klmm) {
  KlMapNodePool* nodepool = (KlMapNodePool*)klmm_alloc(klmm, sizeof (KlMapNodePool));
  if (kl_unlikely(!nodepool)) return NULL;
  nodepool->nodes = NULL;
  nodepool->available = 0;
  nodepool->ref_count = 0;
  nodepool->klmm = klmm;
  return nodepool;
}

void klmapnodepool_delete(KlMapNodePool* nodepool) {
  KlMM* klmm = nodepool->klmm;
  KlMapNode* node = nodepool->nodes;
  while (node) {
    KlMapNode* tmp = node->next;
    klmm_free(klmm, node, sizeof (KlMapNode));
    node = tmp;
  }
  klmm_free(klmm, nodepool, sizeof (KlMapNodePool));
}

void klmapnodepool_shrink(KlMapNodePool* nodepool) {
  size_t freecount = nodepool->available / 2;
  nodepool->available -= freecount;
  KlMapNode* node = nodepool->nodes;
  KlMM* klmm = nodepool->klmm;
  while (freecount--) {
    KlMapNode* tmp = node->next;
    klmm_free(klmm, node, sizeof (KlMapNode));
    node = tmp;
  }
  nodepool->nodes = node;
}
