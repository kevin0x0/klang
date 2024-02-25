#include "klang/include/parse/klsymtab.h"
#include "klang/include/objpool/klsymtabnode_pool.h"

#include <stdlib.h>
#include <string.h>

static inline void klsymtab_node_insert(KlSymTabNode* insertpos, KlSymTabNode* elem);
static inline void klsymtab_init_head_tail(KlSymTab* map);

static inline void klsymtab_init_head_tail(KlSymTab* map) {
  map->head->next = map->tail;
  map->tail->prev = map->head;
  map->head->prev = NULL;
  map->tail->next = NULL;
}

inline static size_t klsymtab_hashing(const KString* key) {
  size_t hashval = 0;
  const char* str = kstring_get_content_const(key);
  const char* endptr = str + (kstring_get_len(key) < 16 ? kstring_get_len(key) : 16);
  for (const char* ptr = str; ptr != endptr; ++ptr) {
    hashval += *ptr * 31;
  }
  return hashval + kstring_get_len(key);
}

static void klsymtab_rehash(KlSymTab* to, KlSymTab* from) {
  size_t to_capacity = to->capacity;
  KlSymTabNode** to_array = to->array;
  size_t mask = to_capacity - 1;
  KlSymTabNode* node = from->head->next;
  KlSymTabNode* end = from->tail;
  while (node != end) {
      KlSymTabNode* tmp = node->next;
      size_t hashval = node->hashval;
      size_t index = hashval & mask;
      /* this means 'node' is the first element that put in this bucket,
       * so this bucket has not added to bucket list yet. */
      if (!to_array[index]) {
        to_array[index] = node;
        klsymtab_node_insert(to->head->next, node);
      } else {
        klsymtab_node_insert(to_array[index]->next, node);
      }
      node = tmp;
    }
  to->size = from->size;
  free(from->array);
  klsymtabnode_pool_deallocate(from->head);
  klsymtabnode_pool_deallocate(from->tail);
  from->array = NULL;
  from->capacity = 0;
  from->size = 0;
}

static bool klsymtab_expand(KlSymTab* map) {
  KlSymTab new_map;
  if (!klsymtab_init(&new_map, map->parent, map->scope))
    return false;
  klsymtab_rehash(&new_map, map);
  map->capacity = new_map.capacity;
  map->size = new_map.size;
  map->head = new_map.head;
  map->tail = new_map.tail;
  map->array = new_map.array;
  return true;
}

inline static size_t pow_of_2_above(size_t num) {
  size_t pow = 8;
  while (pow < num)
    pow <<= 1;

  return pow;
}

bool klsymtab_init(KlSymTab* map, KlSymTab* parent, KlScope scope) {
  if (!map) return false;
  map->parent = parent;
  map->scope = scope;

  size_t capacity = pow_of_2_above(8);
  KlSymTabNode** array = (KlSymTabNode**)malloc(sizeof (KlSymTabNode*) * capacity);
  map->head = klsymtabnode_pool_allocate();
  map->tail = klsymtabnode_pool_allocate();
  if (!array || !map->head || !map->tail) {
    klsymtabnode_pool_deallocate(map->head);
    klsymtabnode_pool_deallocate(map->tail);
    map->array = NULL;
    map->capacity = 0;
    map->size = 0;
    return false;
  }
  klsymtab_init_head_tail(map);
  memset(array, 0, sizeof (KlSymTabNode*) * capacity);
  map->array = array;
  map->capacity = capacity;
  map->size = 0;
  return true;
}

KlSymTab* klsymtab_create(KlSymTab* parent, KlScope scope) {
  KlSymTab* map = (KlSymTab*)malloc(sizeof (KlSymTab));
  if (!map || !klsymtab_init(map, parent, scope)) {
    free(map);
    return NULL;
  }
  return map;
}

void klsymtab_destroy(KlSymTab* map) {
  if (!map) return;
  KlSymTabNode* node = map->head->next;
  KlSymTabNode* end = map->tail;
  while (node != end) {
    KlSymTabNode* tmp = node->next;
    kstring_delete(node->name);
    klsymtabnode_pool_deallocate(node);
    node = tmp;
  }
  free(map->array);
  klsymtabnode_pool_deallocate(map->head);
  klsymtabnode_pool_deallocate(map->tail);
  map->head = NULL;
  map->tail = NULL;
  map->array = NULL;
  map->capacity = 0;
  map->size = 0;
}

void klsymtab_delete(KlSymTab* map) {
  klsymtab_destroy(map);
  free(map);
}

KlSymTabIter klsymtab_insert_move(KlSymTab* map, KString* key) {
  if (map->size >= map->capacity && !klsymtab_expand(map))
    return false;

  KlSymTabNode* new_node = klsymtabnode_pool_allocate();
  if (!new_node)
    return klsymtab_iter_end(map);

  size_t hashval = klsymtab_hashing(key);
  size_t index = (map->capacity - 1) & hashval;
  new_node->name = key;
  new_node->hashval = hashval;
  if (!map->array[index]) {
    map->array[index] = new_node;
    klsymtab_node_insert(map->head->next, new_node);
  } else {
    klsymtab_node_insert(map->array[index]->next, new_node);
  }
  map->size++;
  return new_node;
}

KlSymTabIter klsymtab_search(KlSymTab* map, const KString* key) {
  size_t  hashval = klsymtab_hashing(key);
  size_t mask = map->capacity - 1;
  size_t index = mask & hashval;
  KlSymTabNode* node = map->array[index];
  if (!node) return klsymtab_iter_end(map);
  do {
    if (node->hashval == hashval &&
        kstring_compare(key, node->name) == 0) {
      return node;
    }
    node = node->next;
  } while (node != map->tail && (node->hashval & mask) == index);
  return klsymtab_iter_end(map);
}

KlSymTabIter klsymtab_erase(KlSymTab* map, KlSymTabIter iter) {
  iter->prev->next = iter->next;
  iter->next->prev = iter->prev;
  --map->size;
  size_t index = (map->capacity - 1) & iter->hashval;
  KlSymTabIter next = iter->next;
  if (map->array[index] == iter) {
    if (next == map->tail || (next->hashval & (map->capacity - 1)) != index) {
      map->array[index] = NULL;
    } else {
      map->array[index] = next;
    }
  }
  kstring_delete(iter->name);
  klsymtabnode_pool_deallocate(iter);
  return next;
}

static inline void klsymtab_node_insert(KlSymTabNode* insertpos, KlSymTabNode* node) {
  node->prev = insertpos->prev;
  insertpos->prev->next = node;
  node->next = insertpos;
  insertpos->prev = node;
}
