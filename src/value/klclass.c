#include "include/value/klclass.h"
#include "include/misc/klutils.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include <stddef.h>
#include <string.h>

static KlGCObject* klclass_propagate(KlClass* klclass, KlMM* klmm, KlGCObject* gclist);
static void klclass_delete(KlClass* klclass, KlMM* klmm);

static const KlGCVirtualFunc klclass_gcvfunc = { .destructor = (KlGCDestructor)klclass_delete, .propagate = (KlGCProp)klclass_propagate, .after = NULL };

static KlClassSlot* klclass_getfreeslot(KlClass* klclass);
static bool klclass_rehash(KlClass* klclass, KlMM* klmm);
static KlClassSlot* klclass_add_rehash(KlClass* klclass, const KlString* key);


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor) {
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!klclass)) return NULL;
  capacity = (size_t)1 << capacity;
  klclass->mask = capacity - 1;
  klclass->lastfree = capacity;
  klclass->constructor = constructor ? constructor : klclass_default_constructor;
  klclass->constructor_data = constructor_data;
  klclass->attroffset = attroffset;
  klclass->nlocal = 0;
  klclass->is_final = false;
  klclass->slots = (KlClassSlot*)klmm_alloc(klmm, capacity * sizeof (KlClassSlot));
  if (kl_unlikely(!klclass->slots)) {
    klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }
  for (size_t i = 0; i < capacity; ++i) {
    klclass->slots[i].key = NULL;
    klclass->slots[i].next = NULL;
  }
  klmm_gcobj_enable(klmm, klmm_to_gcobj(klclass), &klclass_gcvfunc);
  return klclass;
}

KlClass* klclass_inherit(KlMM* klmm, const KlClass* parent) {
  size_t capacity = klclass_capacity(parent);
  KlClassSlot* array = (KlClassSlot*)klmm_alloc(klmm, capacity * sizeof (KlClassSlot));
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!array || !klclass)) {
    if (array) klmm_free(klmm, array, capacity * sizeof (KlClassSlot));
    if (klclass) klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }

  KlClassSlot* end = array + capacity;
  KlClassSlot* slotbegin = parent->slots;
  for (KlClassSlot* itr = array, * slot = slotbegin; itr != end; ++itr, ++slot) {
    if ((itr->key = slot->key)) {
      klvalue_setvalue(&itr->value, &slot->value);
      itr->next = slot->next ? array + (slot->next - slotbegin) : NULL;
    } else {
      itr->next = NULL;
    }
  }
  klclass->constructor = parent->constructor;
  klclass->constructor_data = parent->constructor_data;
  klclass->mask = capacity - 1;
  klclass->lastfree = parent->lastfree;
  klclass->attroffset = parent->attroffset;
  klclass->nlocal = parent->nlocal;
  klclass->is_final = false;
  klclass->slots = array;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(klclass), &klclass_gcvfunc);
  return klclass;
}

static bool klclass_rehash(KlClass* klclass, KlMM* klmm) {
  KlClassSlot* oldslots = klclass->slots;
  KlClassSlot* endslots = oldslots + klclass_capacity(klclass);
  size_t new_capacity = klclass_capacity(klclass) * 2;
  klclass->slots = (KlClassSlot*)klmm_alloc(klmm, new_capacity * sizeof (KlClassSlot)); 
  if (kl_unlikely(!klclass->slots)) {
    klclass->slots = oldslots;
    return false;
  }

  for (size_t i = 0; i < new_capacity; ++i) {
    klclass->slots[i].key = NULL;
    klclass->slots[i].next = NULL;
  }

  klclass->mask = new_capacity - 1;
  klclass->lastfree = new_capacity; /* reset lastfree */
  for (KlClassSlot* slot = oldslots; slot != endslots; ++slot) {
    KlClassSlot* inserted = klclass_add_rehash(klclass, slot->key);
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

static KlClassSlot* klclass_add_rehash(KlClass* klclass, const KlString* key) {
  kl_assert(!klstring_islong(key), "");

  size_t mask = klclass_mask(klclass);
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (!slot->key) { /* slot is empty */
    slot->key = key;
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

KlException klclass_newfield(KlClass* klclass, KlMM* klmm, const KlString* key, const KlValue* value) {
  size_t mask = klclass_mask(klclass);
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (kl_likely(!slot->key)) { /* slot is empty */
    slot->key = key;
    klvalue_setvalue(&slot->value, value);
    return KL_E_NONE;
  }
  /* this slot is not empty */
  /* find */
  KlClassSlot* findslot = slot;
  do {
    if (findslot->key == key) {
      klvalue_setvalue(&findslot->value, value);
      return KL_E_NONE;
    }
    findslot = findslot->next;
  } while (findslot);

  /* not found, check whether it is a long string */
  if (kl_unlikely(klstring_islong(key)))  /* do not insert long string */
    return KL_E_INVLD;
  /* else is short string, insert it */
  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  if (kl_unlikely(!newslot)) { /* no slot */
    if (kl_unlikely(!klclass_rehash(klclass, klmm)))
      return KL_E_OOM;
    KlClassSlot* slot = klclass_add_rehash(klclass, key);
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

KlException klclass_addnormal_nosearch(KlClass* klclass, KlMM* klmm, const KlString* key, const KlValue* value) {
  size_t mask = klclass_mask(klclass);
  size_t index = klstring_hash(key) & mask;
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* slot = &slots[index];
  if (kl_likely(!slot->key)) { /* slot is empty */
    slot->key = key;
    klvalue_setvalue_withtag(&slot->value, value, KLCLASS_TAG_NORMAL);
    return KL_E_NONE;
  }

  /* this slot is not empty, check whether the key is a long string */
  if (kl_unlikely(klstring_islong(key)))  /* do not insert long string */
    return KL_E_INVLD;

  KlClassSlot* newslot = klclass_getfreeslot(klclass);
  if (kl_unlikely(!newslot)) { /* no slot */
    if (kl_unlikely(!klclass_rehash(klclass, klmm)))
      return KL_E_OOM;
    KlClassSlot* slot = klclass_add_rehash(klclass, key);
    klvalue_setvalue_withtag(&slot->value, value, KLCLASS_TAG_NORMAL);
    return KL_E_NONE;
  }
  size_t oldindex = klstring_hash(slot->key) & mask;
  if (oldindex == index) {
    newslot->key = key;
    newslot->next = slot->next;
    slot->next = newslot;
    klvalue_setvalue_withtag(&newslot->value, value, KLCLASS_TAG_NORMAL);
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
    klvalue_setvalue_withtag(&slot->value, value, KLCLASS_TAG_NORMAL);
    return KL_E_NONE;
  }
}

static KlGCObject* klclass_propagate(KlClass* klclass, KlMM* klmm, KlGCObject* gclist) {
  kl_unused(klmm);
  KlClassSlot* slots = klclass->slots;
  KlClassSlot* end = slots + klclass_capacity(klclass);
  for (KlClassSlot* itr = slots; itr != end; ++itr) {
    if (itr->key) {
      klmm_gcobj_mark(klmm_to_gcobj(itr->key), gclist);
      if (klvalue_collectable(&itr->value))
        klmm_gcobj_mark(klvalue_getgcobj(&itr->value), gclist);
    }
  }
  return gclist;
}

static void klclass_delete(KlClass* klclass, KlMM* klmm) {
  klmm_free(klmm, klclass->slots, klclass_capacity(klclass) * sizeof (KlClassSlot));
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
  klmm_gcobj_mark(klmm_to_gcobj(object->klclass), gclist);
  KlValue* attrs = klobject_attrs(object);
  KlUnsigned nlocal = object->klclass->nlocal;
  for (KlUnsigned i = 0; i < nlocal; ++i) {
    if (klvalue_collectable(&attrs[i]))
        klmm_gcobj_mark(klvalue_getgcobj(&attrs[i]), gclist);
  }
  return gclist;
}

static void klobject_delete(KlObject* object, KlMM* klmm) {
  klobject_free(object, klmm);
}
