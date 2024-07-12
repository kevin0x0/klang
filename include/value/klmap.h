#ifndef _KLANG_INCLUDE_KLMAP_H_
#define _KLANG_INCLUDE_KLMAP_H_

#include "include/mm/klmm.h"
#include "include/value/klvalue.h"
#include "include/value/klstring.h"

#include <stddef.h>
#include <stdbool.h>

#define KLMAP_OPT_WEAKKEY   (klbit(0))
#define KLMAP_OPT_WEAKVAL   (klbit(1))

#define klmap_slot_setvalue(slot, val)  klvalue_setvalue(&(slot)->value, (val))

typedef struct tagKlMapSlot KlMapSlot;
typedef struct tagKlMapNodePool KlMapNodePool;

struct tagKlMapSlot {
  KlValue key;
  KlValue value;
  KlMapSlot* next;
  size_t hash;
};

typedef struct tagKlMap {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlMapSlot* slots;
  size_t lastfree;
  size_t capacity;
  size_t size;
  unsigned option;
} KlMap;


KlMap* klmap_create(KlMM* klmm, size_t capacity);

static inline size_t klmap_size(const KlMap* map);
static inline size_t klmap_capacity(const KlMap* map);
static inline void klmap_assignoption(KlMap* map, unsigned option);
static inline void klmap_setoption(KlMap* map, unsigned bit);
static inline void klmap_clroption(KlMap* map, unsigned bit);

bool klmap_insert(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value);
bool klmap_insert_new(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value);
KlMapSlot* klmap_search(const KlMap* map, const KlValue* key);
KlMapSlot* klmap_searchint(const KlMap* map, KlInt key);
KlMapSlot* klmap_searchstring(const KlMap* map, const KlString* str);
KlMapSlot* klmap_insertstring(KlMap* map, KlMM* klmm, const KlString* str, const KlValue* val);
void klmap_erase(KlMap* map, KlMapSlot* slot);
void klmap_makeempty(KlMap* map);

static inline void klmap_index(const KlMap* map, const KlValue* key, KlValue* val);
static inline bool klmap_indexas(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* val);


static inline void klmap_index(const KlMap* map, const KlValue* key, KlValue* val) {
  const KlMapSlot* slot = klmap_search(map, key);
  slot ? klvalue_setvalue(val, &slot->value) : klvalue_setnil(val);
}

static inline bool klmap_indexas(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* val) {
  KlMapSlot* slot = klmap_search(map, key);
  if (slot) {
    klvalue_setvalue(&slot->value, val);
    return true;
  } else {
    return klmap_insert_new(map, klmm, key, val);
  }
}

static inline size_t klmap_size(const KlMap* map) {
  return map->size;
}

static inline size_t klmap_capacity(const KlMap* map) {
  return map->capacity;
}

static inline void klmap_assignoption(KlMap* map, unsigned option) {
  map->option = option;
}

static inline void klmap_setoption(KlMap* map, unsigned bit) {
  map->option |= bit;
}

static inline void klmap_clroption(KlMap* map, unsigned bit) {
  map->option &= ~bit;
}



#endif
