#include "klang/include/value/klmap.h"
#include "klang/include/objpool/klmapnode_pool.h"
#include "klang/include/value/value.h"
#include "utils/include/kstring/kstring.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>

static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* elem);
static inline void klmap_init_head_tail(KlMap* map);

static inline void klmap_init_head_tail(KlMap* map) {
  map->head->next = map->tail;
  map->tail->prev = map->head;
  map->head->prev = NULL;
  map->tail->next = NULL;
}

inline static size_t klmap_hashing(const KString* key) {
  size_t hashval = 0;
  const char* str = kstring_get_content_const(key);
  const char* endptr = str + (kstring_get_len(key) < 16 ? kstring_get_len(key) : 16);
  for (const char* ptr = str; ptr != endptr; ++ptr) {
    hashval += *ptr * 31;
  }
  return hashval + kstring_get_len(key);
}

static void klmap_rehash(KlMap* to, KlMap* from) {
  size_t to_capacity = to->capacity;
  KlMapNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  KlMapNode* node = from->head->next;
  KlMapNode* end = from->tail;
  while (node != end) {
      KlMapNode* tmp = node->next;
      size_t hashval = node->hashval;
      size_t index = hashval & mask;
      /* this means 'node' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
      if (!to_array[index]) {
        to_array[index] = node;
        klmap_node_insert(to->head->next, node);
      } else {
        klmap_node_insert(to_array[index]->next, node);
      }
      node = tmp;
    }
  to->size = from->size;
  free(from->array);
  klmapnode_pool_deallocate(from->head);
  klmapnode_pool_deallocate(from->tail);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool klmap_expand(KlMap* map) {
  KlMap new_map;
  if (!klmap_init(&new_map, map->capacity << 1))
    return false;
  klmap_rehash(&new_map, map);
  *map = new_map;
  return true;
}

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool klmap_init(KlMap* map, size_t capacity) {
  if (!map) return false;
  capacity = pow_of_2_above(capacity);
  KlMapNode** array = (KlMapNode**)malloc(sizeof (KlMapNode*) * capacity);
  map->head = klmapnode_pool_allocate();
  map->tail = klmapnode_pool_allocate();
  if (!array || !map->head || !map->tail) {
    klmapnode_pool_deallocate(map->head);
    klmapnode_pool_deallocate(map->tail);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
    return false;
  }
  klmap_init_head_tail(map);
  memset(array, 0, sizeof (KlMapNode*) * capacity);
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

bool klmap_init_copy(KlMap* map, KlMap* src) {
  if (!klmap_init(map, klmap_capacity(src)))
    return false;
  KlMapIter begin = klmap_iter_begin(src);
  KlMapIter end = klmap_iter_end(src);
  for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
    if (!klmap_insert(map, itr->key, itr->value)) {
      klmap_destroy(map);
      return false;
    }
  }
  return true;
}

KlMap* klmap_create_copy(KlMap* src) {
  KlMap* map = (KlMap*)malloc(sizeof (KlMap));
  if (!map || !klmap_init_copy(map, src)) {
    free(map);
    return NULL;
  }
  return map;
}

KlMap* klmap_create(size_t capacity) {
  KlMap* map = (KlMap*)malloc(sizeof (KlMap));
  if (!map || !klmap_init(map, capacity)) {
    free(map);
    return NULL;
  }
  return map;
}

void klmap_destroy(KlMap* map) {
  if (!map) return;
  KlMapNode* node = map->head->next;
  KlMapNode* end = map->tail;
  while (node != end) {
    KlMapNode* tmp = node->next;
    kstring_delete(node->key);
    klmapnode_pool_deallocate(node);
    node = tmp;
  }
  free(map->array);
  klmapnode_pool_deallocate(map->head);
  klmapnode_pool_deallocate(map->tail);
  map->head = NULL;
  map->tail = NULL;
  map->array = NULL;
  map->capacity = 0;
  map->size = 0;
}

void klmap_delete(KlMap* map) {
  klmap_destroy(map);
  free(map);
}

bool klmap_insert_move(KlMap* map, KString* key, KlValue* value) {
  if (map->size >= map->capacity && !klmap_expand(map))
    return false;

  KlMapNode* new_node = klmapnode_pool_allocate();
  if (!new_node) return false;

  size_t hashval = klmap_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->key = key;
  new_node->hashval = hashval;
  new_node->value = value;
  if (!map->array[index]) {
    map->array[index] = new_node;
    klmap_node_insert(map->head->next, new_node);
  } else {
    klmap_node_insert(map->array[index]->next, new_node);
  }
  map->size++;
  return true;
}

KlMapIter klmap_search(KlMap* map, const KString* key) {
  size_t  hashval = klmap_hashing(key);
  size_t mask = map->capacity - 1;
  size_t index = mask & hashval;
  KlMapNode* node = map->array[index];
  if (!node) return klmap_iter_end(map);
  do {
    if (node->hashval == hashval &&
        kstring_compare(key, node->key) == 0) {
      return node;
    }
    node = node->next;
  } while ((node->hashval & mask) == index);
  return klmap_iter_end(map);
}

KlMapIter klmap_erase(KlMap* map, KlMapIter iter) {
  iter->prev->next = iter->next;
  iter->next->prev = iter->prev;
  --map->size;
  size_t index = (map->capacity - 1) & iter->hashval;
  KlMapIter next = iter->next;
  if (map->array[index] == iter) {
    if (next == map->tail || (next->hashval & (map->capacity - 1)) != index) {
      map->array[index] = NULL;
    } else {
      map->array[index] = next;
    }
  }
  kstring_delete(iter->key);
  klmapnode_pool_deallocate(iter);
  return next;
}

static inline void klmap_node_insert(KlMapNode* insertpos, KlMapNode* node) {
  node->prev = insertpos->prev;
  insertpos->prev->next = node;
  node->next = insertpos;
  insertpos->prev = node;
}
