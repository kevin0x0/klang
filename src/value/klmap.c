#include "include/value/klmap.h"
#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include "include/value/klclass.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static KlGCObject* klmap_propagate(KlMap* map, KlMM* klmm, KlGCObject* gclist);
static void klmap_delete(KlMap* map, KlMM* klmm);
static void klmap_post(KlMap* map, KlMM* klmm);

static const KlGCVirtualFunc klmap_gcvfunc = { .propagate = (KlGCProp)klmap_propagate, .destructor = (KlGCDestructor)klmap_delete, .after = (KlGCAfter)klmap_post };


static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* elem);
static inline void klmap_init_head_tail(KlMapNode* head, KlMapNode* tail);

static inline size_t klmap_gethash(const KlValue* key) {
  switch (klvalue_gettype(key)) {
    case KL_STRING: {
      return klstring_hash(klvalue_getobj(key, KlString*));
    }
    case KL_INT: {
      KlInt val = klvalue_getint(key);
      return (val << 16) ^ val;
    }
    case KL_NIL: {
      return klvalue_getnil(key);
    }
    case KL_BOOL: {
      return klvalue_getbool(key);
    }
    case KL_FLOAT: {
      /* NOT PORTABLE */
      kl_static_assert(sizeof (KlFloat) == sizeof (KlInt), "");
      union {
        size_t hash;
        KlFloat floatval;
      } num;
      num.floatval = klvalue_getfloat(key);
      /* +0.0 and -0.0 is equal but have difference binary representations */
      return num.floatval == 0.0 ? 0 : num.hash;
    }
    case KL_CFUNCTION: {
      return ((uintptr_t)klvalue_getcfunc(key) >> 3);
    }
    default: {
      return ((uintptr_t)klvalue_getgcobj(key) >> 3);
    }
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

  klmm_free(klmapnodepool_getmm(map->nodepool), map->array, map->capacity * sizeof (KlMapNode*));
  map->array = new_array;
  map->capacity = new_capacity;
}

static bool klmap_expand(KlMap* map) {
  size_t new_capacity = map->capacity << 1;
  KlMapNode** new_array = klmm_alloc(klmapnodepool_getmm(map->nodepool), new_capacity * sizeof (KlMapNode*));
  if (kl_unlikely(!new_array)) return false;
  for (size_t i = 0; i < new_capacity; ++i)
    new_array[i] = NULL;
  klmap_rehash(map, new_array, new_capacity);
  return true;
}

KlMap* klmap_create(KlClass* mapclass, size_t capacity, KlMapNodePool* nodepool) {
  capacity = ((size_t)1) << capacity;

  KlMM* klmm = klmapnodepool_getmm(nodepool);
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
  map->option = 0;
  klmapnodepool_pin(nodepool);
  klmm_gcobj_enable(klmm, klmm_to_gcobj(map), &klmap_gcvfunc);
  return map;
}

static void klmap_delete(KlMap* map, KlMM* klmm) {
  if (klmap_size(map) != 0)
    klmapnodepool_freelist(map->nodepool, map->head.next, map->tail.prev, klmap_size(map));

  klmapnodepool_unpin(map->nodepool);
  klmm_free(klmm, map->array, map->capacity * sizeof (KlMapNode*));
  klobject_free(klcast(KlObject*, map), klmm);
}

static void klmap_post(KlMap* map, KlMM* klmm) {
  kl_unused(klmm);
  KlMapIter end = klmap_iter_end(map);
  KlMapIter begin = klmap_iter_begin(map);
  for (KlMapIter itr = begin; itr != end;) {
    if ((klvalue_collectable(&itr->key) && klmm_gcobj_isdead(klvalue_getgcobj(&itr->key))) ||
        (klvalue_collectable(&itr->value) && klmm_gcobj_isdead(klvalue_getgcobj(&itr->value)))) {
      itr = klmap_erase(map, itr);
    } else {
      itr = klmap_iter_next(itr);
    }
  }
}

void klmap_makeempty(KlMap* map) {
  klmapnodepool_freelist(map->nodepool, map->head.next, map->tail.prev, map->size);
  size_t capacity = map->capacity;
  KlMapNode** array = map->array;
  for (size_t i = 0; i < capacity; ++i)
    array[i] = NULL;
  klmap_init_head_tail(&map->head, &map->tail);
  map->size = 0;
}

KlMapIter klmap_insert(KlMap* map, const KlValue* key, const KlValue* value) {
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

KlMapIter klmap_search(const KlMap* map, const KlValue* key) {
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

KlMapIter klmap_bucketnext(KlMap* map, size_t bucketid, KlValue* key) {
  kl_assert(klmap_validbucket(map, bucketid), "");
  size_t mask = map->capacity - 1;
  KlMapIter itr = map->array[mask & bucketid];
  do {
    if (klvalue_equal(key, &itr->key)) {
      return itr->next == &map->tail ? NULL : itr->next;
    }
    itr = itr->next;
  } while (itr != &map->tail && (itr->hash & mask) == bucketid);
  return NULL;
}

KlMapIter klmap_searchstring(const KlMap* map, const KlString* str) {
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

KlMapIter klmap_insertstring(KlMap* map, const KlString* str, const KlValue* val) {
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

static KlGCObject* klmap_propagate_nonweak(KlMap* map, KlGCObject* gclist) {
  KlMapIter end = klmap_iter_end(map);
  KlMapIter begin = klmap_iter_begin(map);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    if (klvalue_collectable(&itr->key))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->key), gclist);
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->value), gclist);
  }
  return klobject_propagate_nomm(klcast(KlObject*, map), gclist);
}

static KlGCObject* klmap_propagate_keyweak(KlMap* map, KlGCObject* gclist) {
  KlMapIter end = klmap_iter_end(map);
  KlMapIter begin = klmap_iter_begin(map);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->value), gclist);
  }
  return klobject_propagate_nomm(klcast(KlObject*, map), gclist);
}

static KlGCObject* klmap_propagate_valweak(KlMap* map, KlGCObject* gclist) {
  KlMapIter end = klmap_iter_end(map);
  KlMapIter begin = klmap_iter_begin(map);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    if (klvalue_collectable(&itr->key))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->key), gclist);
  }
  return klobject_propagate_nomm(klcast(KlObject*, map), gclist);
}

static KlGCObject* klmap_propagate(KlMap* map, KlMM* klmm, KlGCObject* gclist) {
  switch (map->option & (KLMAP_OPT_WEAKKEY | KLMAP_OPT_WEAKVAL)) {
    case 0: { /* not a week map */
      return klmap_propagate_nonweak(map, gclist);
    }
    case KLMAP_OPT_WEAKKEY | KLMAP_OPT_WEAKVAL: { /* bother keys and values are weak */
      klmm_gcobj_aftermark(klmm, klmm_to_gcobj(map));
      return klobject_propagate_nomm(klcast(KlObject*, map), gclist);
    }
    case KLMAP_OPT_WEAKKEY: { /* only the keys are weak */
      klmm_gcobj_aftermark(klmm, klmm_to_gcobj(map));
      return klmap_propagate_keyweak(map, gclist);
    }
    case KLMAP_OPT_WEAKVAL: {  /* only the values are week */
      klmm_gcobj_aftermark(klmm, klmm_to_gcobj(map));
      return klmap_propagate_valweak(map, gclist);
    }
    default: {
      kl_assert(false, "unreachable");
      return gclist;
    }
  }
}





KlMapNodePool* klmapnodepool_create(KlMM* klmm) {
  KlMapNodePool* nodepool = (KlMapNodePool*)klmm_alloc(klmm, sizeof (KlMapNodePool));
  if (kl_unlikely(!nodepool)) return NULL;
  nodepool->nodes = NULL;
  nodepool->available = 0;
  nodepool->pincount = 0;
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
