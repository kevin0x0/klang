#ifndef KEVCC_KLANG_INCLUDE_KLMAP_H
#define KEVCC_KLANG_INCLUDE_KLMAP_H

#include "klang/include/value/typedecl.h"
#include "utils/include/kstring/kstring.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct tagKlMapNode {
  KString* key;
  KlValue* value;
  size_t hashval;
  struct tagKlMapNode* next;
  struct tagKlMapNode* prev;
} KlMapNode;

typedef KlMapNode* KlMapIter;

typedef struct tagKlMap {
  KlMapNode** array;
  KlMapNode* head;
  KlMapNode* tail;
  size_t capacity;
  size_t size;
} KlMap;

bool klmap_init(KlMap* map, size_t capacity);
bool klmap_init_copy(KlMap* map, KlMap* src);
KlMap* klmap_create(size_t capacity);
KlMap* klmap_create_copy(KlMap* src);
void klmap_destroy(KlMap* map);
void klmap_delete(KlMap* map);

static inline size_t klmap_size(KlMap* map);
static inline size_t klmap_capacity(KlMap* map);

static inline bool klmap_insert(KlMap* map, const KString* key, KlValue* value);
bool klmap_insert_move(KlMap* map, KString* key, KlValue* value);
KlMapIter klmap_erase(KlMap* map, KlMapIter iter);
KlMapIter klmap_search(KlMap* map, const KString* key);

static inline KlMapIter klmap_iter_begin(KlMap* map);
static inline KlMapIter klmap_iter_end(KlMap* map);
static inline KlMapIter klmap_iter_next(KlMapIter current);


static inline KlMapIter klmap_iter_begin(KlMap* map) {
    return map->head->next;
}

static inline KlMapIter klmap_iter_end(KlMap* map) {
  return map->tail;
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

static inline bool klmap_insert(KlMap* map, const KString* key, KlValue* value) {
  KString* copy = kstring_create_copy(key);
  if (!copy || !klmap_insert_move(map, copy, value)) {
    kstring_delete(copy);
    return false;
  }
  return true;
}


#endif
