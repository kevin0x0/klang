#include "include/value/klclass.h"
#include "include/misc/klutils.h"
#include "include/value/klstring.h"
#include "include/vm/klexception.h"
#include <stddef.h>
#include <string.h>

static KlGCObject* klclass_propagate(KlClass* klclass, KlMM* klmm, KlGCObject* gclist);
static void klclass_delete(KlClass* klclass, KlMM* klmm);

static const KlGCVirtualFunc klclass_gcvfunc = { .destructor = (KlGCDestructor)klclass_delete, .propagate = (KlGCProp)klclass_propagate, .after = NULL };

static KlClassSlot* klclass_getfreeslot(KlClass* klclass);
static bool klclass_rehash(KlClass* klclass, KlMM* klmm);


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor) {
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!klclass)) return NULL;
  klclass->capacity = (size_t)1 << capacity;
  klclass->lastfree = klclass->capacity;
  klclass->constructor = constructor ? constructor : klclass_default_constructor;
  klclass->constructor_data = constructor_data;
  klclass->attroffset = attroffset;
  klclass->nlocal = 0;
  klclass->slots = (KlClassSlot*)klmm_alloc(klmm, klclass->capacity * sizeof (KlClassSlot));
  if (kl_unlikely(!klclass->slots)) {
    klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }
  for (size_t i = 0; i < klclass->capacity; ++i) {
    klvalue_setnil(&klclass->slots[i].value);
    klclass->slots[i].key = NULL;
    klclass->slots[i].next = NULL;
  }
  klmm_gcobj_enable(klmm, klmm_to_gcobj(klclass), &klclass_gcvfunc);
  return klclass;
}

KlClass* klclass_inherit(KlMM* klmm, KlClass* parent) {
  KlClassSlot* array = (KlClassSlot*)klmm_alloc(klmm, parent->capacity * sizeof (KlClassSlot));
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!array || !klclass)) {
    klmm_free(klmm, array, parent->capacity * sizeof (KlClassSlot));
    klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }

  KlClassSlot* slots = parent->slots;
  memcpy(array, slots, parent->capacity * sizeof (KlClassSlot));
  ptrdiff_t offset = array - slots;
  KlClassSlot* end = array + parent->capacity;
  for (KlClassSlot* itr = array; itr != end; ++itr) {
    if (itr->next) itr->next += offset;
  }
  klclass->constructor = parent->constructor;
  klclass->constructor_data = parent->constructor_data;
  klclass->capacity = parent->capacity;
  klclass->lastfree = parent->lastfree;
  klclass->attroffset = parent->attroffset;
  klclass->nlocal = parent->nlocal;
  klclass->slots = array;
  return klclass;
}

static bool klclass_rehash(KlClass* klclass, KlMM* klmm) {
  KlClassSlot* oldslots = klclass->slots;
  KlClassSlot* endslots = oldslots + klclass->capacity;
  size_t new_capacity = klclass->capacity * 2;
  klclass->slots = (KlClassSlot*)klmm_alloc(klmm, new_capacity * sizeof (KlClassSlot)); 
  if (kl_unlikely(!klclass->slots)) {
    klclass->slots = oldslots;
    return false;
  }

  for (size_t i = 0; i < new_capacity; ++i) {
    klvalue_setnil(&klclass->slots[i].value);
    klclass->slots[i].key = NULL;
    klclass->slots[i].next = NULL;
  }

  klclass->capacity = new_capacity;
  klclass->lastfree = new_capacity; /* reset lastfree */
  for (KlClassSlot* slot = oldslots; slot != endslots; ++slot) {
    KlClassSlot* inserted = klclass_add(klclass, klmm, slot->key);
    klvalue_setvalue(&inserted->value, &slot->value);
  }
  klmm_free(klmm, oldslots, (endslots - oldslots) * sizeof (KlClassSlot));
  return true;
}

static KlClassSlot* klclass_getfreeslot(KlClass* klclass) {
  size_t lastfree = klclass->lastfree;
  KlClassSlot* slots = klclass->slots;
  while (lastfree--) {
    if (!slots[lastfree].key) {
      klclass->lastfree = lastfree;
      return &slots[lastfree];
    }
  }
  return NULL;
}

static KlClassSlot* klclass_add_after_rehash(KlClass* klclass, KlString* key) {
  size_t mask = klclass->capacity - 1;
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (!slot->key) { /* slot is empty */
    slot->key = key;
    slot->next = NULL;
    return slot;
  }
  /* this slot is not empty */
  /* just insert, no need to check whether this key is duplicated */
  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  size_t oldindex = klstring_hash(slot->key) & mask;
  if (oldindex == index) {
    newslot->key = key;
    newslot->next = slot->next;
    slot->next = newslot;
    return newslot;
  } else {
    newslot->key = slot->key;
    klvalue_setvalue(&newslot->value, &slot->value);
    newslot->next = slot->next;
    /* correct link */
    KlClassSlot* prevslot = &slots[oldindex];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = newslot;

    slot->key = key;
    slot->next = NULL;
    return slot;
  }
}

KlClassSlot* klclass_get(KlClass* klclass, KlMM* klmm, KlString* key) {
  size_t mask = klclass->capacity - 1;
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (!slot->key) { /* slot is empty */
    slot->key = key;
    slot->next = NULL;
    return slot;
  }
  /* this slot is not empty */
  /* find */
  KlClassSlot* findslot = slot;
  while (findslot) {
    if (findslot->key == key)
      return findslot;
    findslot = findslot->next;
  }

  /* not found, insert new one */
  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  if (!newslot) { /* no slot */
    if (kl_unlikely(!klclass_rehash(klclass, klmm)))
      return NULL;
    return klclass_add_after_rehash(klclass, key);
  }
  size_t oldindex = klstring_hash(slot->key) & mask;
  if (oldindex == index) {
    newslot->key = key;
    newslot->next = slot->next;
    slot->next = newslot;
    return newslot;
  } else {
    newslot->key = slot->key;
    klvalue_setvalue(&newslot->value, &slot->value);
    newslot->next = slot->next;
    /* correct link */
    KlClassSlot* prevslot = &slots[oldindex];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = newslot;

    slot->key = key;
    slot->next = NULL;
    return slot;
  }
}

KlException klclass_newfield(KlClass* klclass, KlMM* klmm, KlString* key, KlValue* value) {
  size_t mask = klclass->capacity - 1;
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (!slot->key) { /* slot is empty */
    slot->key = key;
    slot->next = NULL;
    klvalue_setvalue(&slot->value, value);
    return KL_E_NONE;
  }
  /* this slot is not empty */
  /* find */
  KlClassSlot* findslot = slot;
  while (findslot) {
    if (findslot->key == key) {
      if (klclass_is_local(findslot)) /* local field can not be overwritten */
        return KL_E_OOM;
      klvalue_setvalue(&slot->value, value);
      return KL_E_NONE;
    }
    findslot = findslot->next;
  }

  /* not found, insert new one */
  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  if (!newslot) { /* no slot */
    if (kl_unlikely(!klclass_rehash(klclass, klmm)))
      return KL_E_OOM;
    KlClassSlot* slot = klclass_add_after_rehash(klclass, key);
    klvalue_setvalue(&slot->value, value);
    return KL_E_NONE;
  }
  size_t oldindex = klstring_hash(slot->key) & mask;
  if (oldindex == index) {
    newslot->key = key;
    newslot->next = slot->next;
    slot->next = newslot;
    klvalue_setvalue(&newslot->value, value);
    return KL_E_NONE;
  } else {
    newslot->key = slot->key;
    klvalue_setvalue(&newslot->value, &slot->value);
    newslot->next = slot->next;
    /* correct link */
    KlClassSlot* prevslot = &slots[oldindex];
    while (prevslot->next != slot)
      prevslot = prevslot->next;
    prevslot->next = newslot;

    slot->key = key;
    slot->next = NULL;
    klvalue_setvalue(&slot->value, value);
    return KL_E_NONE;
  }
}

KlClassSlot* klclass_add(KlClass* klclass, KlMM* klmm, KlString* key) {
  size_t mask = klclass->capacity - 1;
  size_t keyindex = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  if (!slots[keyindex].key) {
    slots[keyindex].key = key;
    slots[keyindex].next = NULL;
    return &slots[keyindex];
  }
  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  if (!newslot) { /* no slot */
    if (kl_unlikely(!klclass_rehash(klclass, klmm)))
      return NULL;
    return klclass_add_after_rehash(klclass, key);
  }

  size_t slotindex = klstring_hash(slots[keyindex].key) & mask;
  if (slotindex == keyindex) {
    newslot->key = key;
    newslot->next = slots[keyindex].next;
    slots[keyindex].next = newslot;
    return newslot;
  } else {
    newslot->key = slots[keyindex].key;
    klvalue_setvalue(&newslot->value, &slots[keyindex].value);
    newslot->next = slots[keyindex].next;
    /* correct link */
    KlClassSlot* prevslot = &slots[slotindex];
    while (prevslot->next != &slots[keyindex])
      prevslot = prevslot->next;
    prevslot->next = newslot;

    slots[keyindex].key = key;
    slots[keyindex].next = NULL;
    return &slots[keyindex];
  }
}

KlClassSlot* klclass_find(KlClass* klclass, struct tagKlString* key) {
  size_t keyindex = klstring_hash(key) & (klclass->capacity - 1);
  KlClassSlot* slot = &klclass->slots[keyindex];
  if (klclass_isfree(slot)) return NULL;
  while (slot) {
    if (slot->key == key)
      return slot;
    slot = slot->next;
  }
  return NULL;
}

static KlGCObject* klclass_propagate(KlClass* klclass, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* end = slots + klclass->capacity;
  for (KlClassSlot* itr = slots; itr != end; ++itr) {
    if (itr->key)
      klmm_gcobj_mark_accessible(klmm_to_gcobj(itr->key), gclist);
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(&itr->value), gclist);
  }
  return gclist;
}

static void klclass_delete(KlClass* klclass, KlMM* klmm) {
  klmm_free(klmm, klclass->slots, klclass->capacity * sizeof (KlClassSlot));
  klmm_free(klmm, klclass, sizeof (KlClass));
}










static void klobject_delete(KlObject* object, KlMM* klmm);
static KlGCObject* klobject_propagate(KlObject* object, KlMM* klmm, KlGCObject* gclist);

static KlGCVirtualFunc klobject_gcvfunc = { .destructor = (KlGCDestructor)klobject_delete, .propagate = (KlGCProp)klobject_propagate };


KlException klclass_default_constructor(KlClass* klclass, KlMM* klmm, KlValue* value) {
  KlObject* obj = klclass_objalloc(klclass, klmm);
  if (kl_unlikely(!obj)) return KL_E_OOM;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(obj), &klobject_gcvfunc);
  klvalue_setobj(value, obj, KL_OBJECT);
  return KL_E_NONE;
}

KlObject* klclass_objalloc(KlClass* klclass, KlMM* klmm) {
  KlUnsigned nlocal = klclass->nlocal;
  size_t allocsize = klclass->attroffset + sizeof (KlValue) * nlocal;
  KlObject* obj = (KlObject*)klmm_alloc(klmm, allocsize);
  if (!obj) return NULL;
  KlValue* attrs = klobject_attrs_by_class(obj, klclass);
  for (KlUnsigned i = 0; i < nlocal; ++i)
    klvalue_setnil(&attrs[i]);
  obj->klclass = klclass;
  obj->attrs = attrs;
  obj->size = allocsize;
  return obj;
}

static KlGCObject* klobject_propagate(KlObject* object, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  klmm_gcobj_mark_accessible(klmm_to_gcobj(object->klclass), gclist);
  KlValue* attrs = klobject_attrs(object);
  KlUnsigned nlocal = object->klclass->nlocal;
  for (KlUnsigned i = 0; i < nlocal; ++i) {
    if (klvalue_collectable(&attrs[i]))
        klmm_gcobj_mark_accessible(klvalue_getgcobj(&attrs[i]), gclist);
  }
  return gclist;
}

static void klobject_delete(KlObject* object, KlMM* klmm) {
  klobject_free(object, klmm);
}
