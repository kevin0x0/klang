#ifndef _KLANG_INCLUDE_KLMAP_H_
#define _KLANG_INCLUDE_KLMAP_H_

#include "include/misc/klutils.h"
#include "include/mm/klmm.h"
#include "include/value/klvalue.h"
#include "include/value/klstring.h"

#include <stddef.h>
#include <stdbool.h>

#define KLMAP_OPT_WEAKKEY   (klbit(0))
#define KLMAP_OPT_WEAKVAL   (klbit(1))


#define KLMAP_KEYTAG_EMPTY    (0)
#define KLMAP_KEYTAG_SLAVE    ((unsigned)klbit(0))
#define KLMAP_KEYTAG_MASTER   ((unsigned)klbit(1))

#define klmap_slot_setvalue(slot, val)  klvalue_setvalue(&(slot)->value, (val))

#define klmap_emptyslot(slot)   (klvalue_gettag(&(slot)->key) == KLMAP_KEYTAG_EMPTY)
#define klmap_masterslot(slot)  (klvalue_gettag(&(slot)->key) == KLMAP_KEYTAG_MASTER)
#define klmap_slaveslot(slot)   (klvalue_gettag(&(slot)->key) == KLMAP_KEYTAG_SLAVE)

typedef struct tagKlMapSlot KlMapSlot;

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
  unsigned option;
} KlMap;


KlMap* klmap_create(KlMM* klmm, size_t capacity);

static inline size_t klmap_capacity(const KlMap* map);
static inline void klmap_assignoption(KlMap* map, unsigned option);
static inline void klmap_setoption(KlMap* map, unsigned bit);
static inline void klmap_clroption(KlMap* map, unsigned bit);

static inline size_t klmap_getinthash(KlInt key);
static inline size_t klmap_gethash(const KlValue* key);

bool klmap_insert(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value);
bool klmap_insert_new(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value);
bool klmap_insertstring(KlMap* map, KlMM* klmm, const KlString* str, const KlValue* value);
static inline KlMapSlot* klmap_search(const KlMap* map, const KlValue* key);
static inline KlMapSlot* klmap_searchint(const KlMap* map, KlInt key);
static inline KlMapSlot* klmap_searchstring(const KlMap* map, const KlString* str);
void klmap_erase(KlMap* map, KlMapSlot* slot);
void klmap_makeempty(KlMap* map);

static inline bool klmap_iter_valid(const KlMap* map, size_t index);
static inline size_t klmap_iter_begin(const KlMap* map);
static inline size_t klmap_iter_next(const KlMap* map, size_t prev);
static inline size_t klmap_iter_end(const KlMap* map);
static inline const KlValue* klmap_iter_getvalue(const KlMap* map, size_t index);
static inline const KlValue* klmap_iter_getkey(const KlMap* map, size_t index);

static inline void klmap_index(const KlMap* map, const KlValue* key, KlValue* val);
static inline bool klmap_indexas(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* val);


static inline size_t klmap_getinthash(KlInt key) {
  return (key << 16) ^ key;
}

static inline size_t klmap_gethash(const KlValue* key) {
  switch (klvalue_gettype(key)) {
    case KL_STRING: {
      return klstring_hash(klvalue_getobj(key, KlString*));
    }
    case KL_INT: {
      return klmap_getinthash(klvalue_getint(key));
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

static inline KlMapSlot* klmap_search(const KlMap* map, const KlValue* key) {
  size_t hash = klmap_gethash(key);
  KlMapSlot* slot = &map->slots[hash & (map->capacity - 1)];
  if (!klmap_masterslot(slot)) return NULL;
  do {
    if (hash == slot->hash && klvalue_equal(&slot->key, key))
      return slot;
    slot = slot->next;
  } while (slot);
  return NULL;
}

static inline KlMapSlot* klmap_searchstring(const KlMap* map, const KlString* str) {
  size_t hash = klstring_hash(str);
  KlMapSlot* slot = &map->slots[hash & (map->capacity - 1)];
  if (!klmap_masterslot(slot)) return NULL;
  do {
    if (klvalue_checktype(&slot->key, KL_STRING) &&
        klvalue_getobj(&slot->key, KlString*) == str)
      return slot;
    slot = slot->next;
  } while (slot);
  return NULL;
}

static inline KlMapSlot* klmap_searchint(const KlMap* map, KlInt key) {
  size_t hash = klmap_getinthash(key);
  KlMapSlot* slot = &map->slots[hash & (map->capacity - 1)];
  if (!klmap_masterslot(slot)) return NULL;
  do {
    if (klvalue_checktype(&slot->key, KL_INT) &&
        klvalue_getint(&slot->key) == key)
      return slot;
    slot = slot->next;
  } while (slot);
  return NULL;
}

static inline bool klmap_iter_valid(const KlMap* map, size_t index) {
  return index == klmap_capacity(map) || (index < klmap_capacity(map) && !klmap_emptyslot(&map->slots[index]));
}

static inline size_t klmap_iter_begin(const KlMap* map) {
  for (size_t i = 0; i  < map->capacity; ++i) {
    if (!klmap_emptyslot(&map->slots[i]))
      return i;
  }
  return map->capacity;
}

static inline size_t klmap_iter_next(const KlMap* map, size_t prev) {
  kl_unused(map);
  kl_assert(klmap_capacity(map) >= prev, "already end");
  for (size_t i = prev + 1; i  < map->capacity; ++i) {
    if (!klmap_emptyslot(&map->slots[i]))
      return i;
  }
  return map->capacity;
}

static inline size_t klmap_iter_end(const KlMap* map) {
  return map->capacity;
}

static inline const KlValue* klmap_iter_getvalue(const KlMap* map, size_t index) {
  kl_assert(!klmap_emptyslot(&map->slots[index]), "not a valid index");
  return &map->slots[index].value;
}

static inline const KlValue* klmap_iter_getkey(const KlMap* map, size_t index) {
  kl_assert(!klmap_emptyslot(&map->slots[index]), "not a valid index");
  return &map->slots[index].key;
}

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
