#include "include/value/klmap.h"
#include "include/value/klvalue.h"
#include "include/mm/klmm.h"
#include "include/misc/klutils.h"

#include <stdlib.h>
#include <string.h>


static KlGCObject* klmap_propagate(KlMap* map, KlMM* klmm, KlGCObject* gclist);
static void klmap_delete(KlMap* map, KlMM* klmm);
static void klmap_post(KlMap* map, KlMM* klmm);

static const KlGCVirtualFunc klmap_gcvfunc = { .propagate = (KlGCProp)klmap_propagate, .destructor = (KlGCDestructor)klmap_delete, .after = (KlGCAfter)klmap_post };


static KlMapSlot* klmap_getfreeslot(KlMap* map);
static bool klmap_rehash(KlMap* map, KlMM* klmm);
static void klmap_insert_rehash(KlMap* map, const KlValue* key, const KlValue* value, size_t hash);

static void klmap_correctlastfree(KlMap* map, size_t newemptyslotindex) {
  if (map->lastfree <= newemptyslotindex)
    map->lastfree = newemptyslotindex + 1;
}


KlMap* klmap_create(KlMM* klmm, size_t capacity) {
  KlMap* map = (KlMap*)klmm_alloc(klmm, sizeof (KlMap));
  if (kl_unlikely(!map)) return NULL;
  capacity = (size_t)1 << capacity;
  map->capacity = capacity;
  map->lastfree = capacity;
  map->option = 0;
  KlMapSlot* slots = (KlMapSlot*)klmm_alloc(klmm, capacity * sizeof (KlMapSlot));
  if (kl_unlikely(!slots)) {
    klmm_free(klmm, map, sizeof (KlMap));
    return NULL;
  }
  map->slots = slots;
  for (size_t i = 0; i < map->capacity; ++i) {
    klvalue_settag(&slots[i].key, KLMAP_KEYTAG_EMPTY);
    slots[i].next = NULL;
  }
  klmm_gcobj_enable(klmm, klmm_to_gcobj(map), &klmap_gcvfunc);
  return map;
}

static bool klmap_rehash(KlMap* map, KlMM* klmm) {
  size_t new_capacity = map->capacity * 2;
  KlMapSlot* slots = (KlMapSlot*)klmm_alloc(klmm, new_capacity * sizeof (KlMapSlot)); 
  if (kl_unlikely(!slots)) return false;

  for (size_t i = 0; i < new_capacity; ++i) {
    klvalue_settag(&slots[i].key, KLMAP_KEYTAG_EMPTY);
    slots[i].next = NULL;
  }

  KlMapSlot* oldslots = map->slots;
  KlMapSlot* endslots = oldslots + map->capacity;
  map->slots = slots;
  map->capacity = new_capacity;
  map->lastfree = new_capacity; /* reset lastfree */
  for (KlMapSlot* slot = oldslots; slot != endslots; ++slot)
    klmap_insert_rehash(map, &slot->key, &slot->value, slot->hash);

  klmm_free(klmm, oldslots, (endslots - oldslots) * sizeof (KlMapSlot));
  return true;
}

KlMapSlot* klmap_searchlstring(const KlMap* map, const KlString* str) {
  size_t hash = klstring_hash(str);
  KlMapSlot* slot = &map->slots[hash & (map->capacity - 1)];
  if (!klmap_masterslot(slot)) return NULL;
  do {
    if (klvalue_checktype(&slot->key, KL_LSTRING) &&
        klstring_lequal(klvalue_getobj(&slot->key, KlString*), str))
      return slot;
    slot = slot->next;
  } while (slot);
  return NULL;
}

static KlMapSlot* klmap_getfreeslot(KlMap* map) {
  size_t lastfree = map->lastfree;
  KlMapSlot* slots = map->slots;
  while (lastfree--) {
    if (klmap_emptyslot(&slots[lastfree])) {
      map->lastfree = lastfree;
      return &slots[lastfree];
    }
  }
  return NULL;
}

static void klmap_insert_rehash(KlMap* map, const KlValue* key, const KlValue* value, size_t hash) {
  size_t mask = map->capacity - 1;
  size_t index = hash & mask;
  KlMapSlot* slots = map->slots;
  KlMapSlot* slot = &slots[index];
  if (klmap_emptyslot(slot)) { /* slot is empty */
    klvalue_setvalue_withtag(&slot->key, key, KLMAP_KEYTAG_MASTER);
    klvalue_setvalue(&slot->value, value);
    slot->hash = hash;
    return;
  }
  /* this slot is not empty */
  /* just insert, no need to check whether this key is duplicated */
  KlMapSlot* newslot = klmap_getfreeslot(map);
  if (klmap_masterslot(slot)) {
    klvalue_setvalue_withtag(&newslot->key, key, KLMAP_KEYTAG_SLAVE);
    klvalue_setvalue(&newslot->value, value);
    newslot->hash = hash;

    newslot->next = slot->next;
    slot->next = newslot;
  } else {
    kl_assert(klmap_slaveslot(slot), "");
    klvalue_setvalue(&newslot->key, &slot->key);
    klvalue_setvalue(&newslot->value, &slot->value);
    newslot->hash = slot->hash;
    newslot->next = slot->next;
    /* correct link */
    KlMapSlot* prevslot = &slots[slot->hash & mask];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = newslot;

    klvalue_setvalue_withtag(&slot->key, key, KLMAP_KEYTAG_MASTER);
    klvalue_setvalue(&slot->value, value);
    slot->hash = hash;
    slot->next = NULL;
  }
}

bool klmap_insert_hash(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value, size_t hash) {
  size_t mask = klmap_mask(map);
  size_t index = hash & mask;
  KlMapSlot* slots = map->slots;
  KlMapSlot* slot = &slots[index];
  if (klmap_emptyslot(slot)) {  /* slot is empty */
    klvalue_setvalue_withtag(&slot->key, key, KLMAP_KEYTAG_MASTER);
    klvalue_setvalue(&slot->value, value);
    slot->hash = hash;
    return true;
  }

  KlMapSlot* newslot = klmap_getfreeslot(map);
  if (!newslot) { /* no slot */
    if (kl_unlikely(!klmap_rehash(map, klmm)))
      return false;
    klmap_insert_rehash(map, key, value, hash);
    return true;
  }
  if (klmap_masterslot(slot)) {
    klvalue_setvalue_withtag(&newslot->key, key, KLMAP_KEYTAG_SLAVE);
    klvalue_setvalue(&newslot->value, value);
    newslot->hash = hash;
    newslot->next = slot->next;
    slot->next = newslot;
    return true;
  } else {
    kl_assert(klmap_slaveslot(slot), "");
    klvalue_setvalue(&newslot->key, &slot->key);
    klvalue_setvalue(&newslot->value, &slot->value);
    newslot->next = slot->next;
    newslot->hash = slot->hash;
    /* correct link */
    KlMapSlot* prevslot = &slots[slot->hash & mask];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = newslot;

    klvalue_setvalue_withtag(&slot->key, key, KLMAP_KEYTAG_MASTER);
    klvalue_setvalue(&slot->value, value);
    slot->hash = hash;
    slot->next = NULL;
    return true;
  }
}

bool klmap_insert_new(KlMap* map, KlMM* klmm, const KlValue* key, const KlValue* value) {
  return klmap_insert_hash(map, klmm, key, value, klmap_gethash(key));
}

static KlGCObject* klmap_propagate_nonweak(KlMap* map, KlGCObject* gclist) {
  KlMapSlot* slots = map->slots;
  KlMapSlot* end = slots + map->capacity;
  for (KlMapSlot* itr = slots; itr != end; ++itr) {
    if (klmap_emptyslot(itr)) continue;
    if (klvalue_collectable(&itr->key))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->key), gclist);
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->value), gclist);
  }
  return gclist;
}

static KlGCObject* klmap_propagate_keyweak(KlMap* map, KlGCObject* gclist) {
  KlMapSlot* slots = map->slots;
  KlMapSlot* end = slots + map->capacity;
  for (KlMapSlot* itr = slots; itr != end; ++itr) {
    if (klmap_emptyslot(itr)) continue;
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->value), gclist);
  }
  return gclist;
}

static KlGCObject* klmap_propagate_valweak(KlMap* map, KlGCObject* gclist) {
  KlMapSlot* slots = map->slots;
  KlMapSlot* end = slots + map->capacity;
  for (KlMapSlot* itr = slots; itr != end; ++itr) {
    if (klmap_emptyslot(itr)) continue;
    if (klvalue_collectable(&itr->key))
      klmm_gcobj_mark(klvalue_getgcobj(&itr->key), gclist);
  }
  return gclist;
}

static KlGCObject* klmap_propagate(KlMap* map, KlMM* klmm, KlGCObject* gclist) {
  switch (map->option & (KLMAP_OPT_WEAKKEY | KLMAP_OPT_WEAKVAL)) {
    case 0: { /* not a week map */
      return klmap_propagate_nonweak(map, gclist);
    }
    case KLMAP_OPT_WEAKKEY | KLMAP_OPT_WEAKVAL: { /* both keys and values are weak */
      klmm_gcobj_aftermark(klmm, klmm_to_gcobj(map));
      return gclist;
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

void klmap_erase(KlMap* map, KlMapSlot* slot) {
  size_t index = slot - map->slots;
  if (klmap_masterslot(slot)) {
    if (slot->next) {
      KlMapSlot* next = slot->next;
      klvalue_setvalue_withtag(&slot->key, &next->key, KLMAP_KEYTAG_MASTER);
      klvalue_setvalue(&slot->value, &next->value);
      slot->hash = next->hash;
      slot->next = next->next;
      klvalue_settag(&next->key, KLMAP_KEYTAG_EMPTY);
      next->next = NULL;
      klmap_correctlastfree(map, next - map->slots);
    } else {
      klvalue_settag(&slot->key, KLMAP_KEYTAG_EMPTY);
      slot->next = NULL;
      klmap_correctlastfree(map, index);
    }
  } else {
    /* correct link */
    KlMapSlot* prevslot = &map->slots[slot->hash & (map->capacity - 1)];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = slot->next;

    klvalue_settag(&slot->key, KLMAP_KEYTAG_EMPTY);
    slot->next = NULL;
    klmap_correctlastfree(map, index);
  }
}

static void klmap_post(KlMap* map, KlMM* klmm) {
  kl_unused(klmm);
  KlMapSlot* slots = map->slots;
  KlMapSlot* end = slots + map->capacity;
  for (KlMapSlot* slot = slots; slot != end; ++slot) {
    while (!klmap_emptyslot(slot) &&
            ((klvalue_collectable(&slot->key) && klmm_gcobj_isdead(klvalue_getgcobj(&slot->key))) ||
             (klvalue_collectable(&slot->value) && klmm_gcobj_isdead(klvalue_getgcobj(&slot->value))))) {
      klmap_erase(map, slot);
    }
  }
}

static void klmap_delete(KlMap* map, KlMM* klmm) {
  klmm_free(klmm, map->slots, map->capacity * sizeof (KlMapSlot));
  klmm_free(klmm, map, sizeof (KlMap));
}
