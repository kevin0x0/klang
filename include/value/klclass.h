#ifndef _KLANG_INCLUDE_VALUE_KLCLASS_H_
#define _KLANG_INCLUDE_VALUE_KLCLASS_H_

#include "include/common/kltypes.h"
#include "include/misc/klutils.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include <stddef.h>

#define KLCLASS_TAG_NORMAL      klcast(KlUnsigned, 0)
#define KLCLASS_TAG_METHOD      klcast(KlUnsigned, klbit(0))
#define KLCLASS_TAG_LOCAL       klcast(KlUnsigned, klbit(1))

#define klclass_slot_isfree(slot)             ((slot)->key == NULL)
#define klclass_slot_is_local(slot)           (klvalue_testtag(&(slot)->value, KLCLASS_TAG_LOCAL))
#define klclass_slot_is_shared(slot)          (!klclass_is_local((slot)))
#define klclass_slot_is_method(slot)          (klvalue_testtag(&(slot)->value, KLCLASS_TAG_METHOD))
#define klclass_slot_setshared(slot, val)     (klvalue_setvalue_withtag(&(slot)->value, (val), KLCLASS_TAG_NORMAL))


typedef struct tagKlClassSlot KlClassSlot;
typedef struct tagKlClass KlClass;
typedef struct tagKlObject KlObject;
typedef KlException (*KlObjectConstructor)(KlClass* klclass, KlMM* klmm, KlValue* result);

struct tagKlClassSlot {
  KlValue value;
  const KlString* key;
  KlClassSlot* next;
};

struct tagKlClass {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlClassSlot* slots;
  KlObjectConstructor constructor;
  void* constructor_data; /* used by constructor */
  size_t mask;
  size_t lastfree;
  size_t attroffset;
  KlUnsigned nlocal;
  bool is_final;
};

#define KLOBJECT_TAIL                         KlValue _klobject_valarray[]

struct tagKlObject {
  KL_DERIVE_FROM(KlGCObject, _gcbase_);
  KlClass* klclass;
  KlValue* attrs;
  size_t size;
};


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor);
KlClass* klclass_inherit(KlMM* klmm, const KlClass* parent);

KlException klclass_default_constructor(KlClass* klclass, KlMM* klmm, KlValue* value);
static inline void* klclass_constructor_data(KlClass* klclass);
KlObject* klclass_objalloc(KlClass* klclass, KlMM* klmm);
static inline KlException klclass_new_object(KlClass* klclass, KlMM* klmm, KlValue* value);

static inline KlClassSlot* klclass_find(const KlClass* klclass, const KlString* key);
KlException klclass_addnormal_nosearch(KlClass* klclass, KlMM* klmm, const KlString* key, const KlValue* value);
KlException klclass_newfield(KlClass* klclass, KlMM* klmm, const KlString* key, const KlValue* result);

static inline size_t klclass_newlocal_anonymous(KlClass* klclass);
static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, const KlString* key);
static inline KlException klclass_newshared_normal(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value);
static inline KlException klclass_newshared_method(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value);


static inline void klclass_final(KlClass* klclass);
static inline bool klclass_isfinal(KlClass* klclass);
static inline KlUnsigned klclass_nlocal(KlClass* klclass);

static inline size_t klclass_capacity(const KlClass* klclass);
static inline size_t klclass_mask(const KlClass* klclass);

static inline size_t klclass_capacity(const KlClass* klclass) {
  return klclass->mask + 1;
}

static inline size_t klclass_mask(const KlClass* klclass) {
  return klclass->mask;
}

static inline void klclass_final(KlClass* klclass) {
  klclass->is_final = true;
}

static inline bool klclass_isfinal(KlClass* klclass) {
  return klclass->is_final;
}

static inline KlUnsigned klclass_nlocal(KlClass* klclass) {
  return klclass->nlocal;
}

static inline void* klclass_constructor_data(KlClass* klclass) {
  return klclass->constructor_data;
}

static inline KlException klclass_new_object(KlClass* klclass, KlMM* klmm, KlValue* value) {
  return klclass->constructor(klclass, klmm, value);
}

static inline size_t klclass_newlocal_anonymous(KlClass* klclass) {
  return klclass->nlocal++;
}

static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, const KlString* key) {
  KlValue localid;
  klvalue_setint_withtag(&localid, klclass->nlocal++, KLCLASS_TAG_LOCAL);
  /* if failed, we don't decrease the klclass->nlocal */
  return klclass_newfield(klclass, klmm, key, &localid);
}

static inline KlException klclass_newshared_normal(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value) {
  klvalue_settag(value, KLCLASS_TAG_NORMAL);
  return klclass_newfield(klclass, klmm, key, value);
}

static inline KlException klclass_newshared_method(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value) {
  klvalue_settag(value, KLCLASS_TAG_METHOD);
  return klclass_newfield(klclass, klmm, key, value);
}

static inline KlClassSlot* klclass_find(const KlClass* klclass, const KlString* key) {
  size_t keyindex = klstring_hash(key) & klclass_mask(klclass);
  KlClassSlot* slot = &klclass->slots[keyindex];
  do {
    if (slot->key == key)
      return slot;
    slot = slot->next;
  } while (slot);
  return NULL;
}




struct tagKlObjectInstance {
  KL_DERIVE_FROM(KlObject, _gcbase_);
  KLOBJECT_TAIL;
};

#define klobject_attrarrayoffset(type)        (offsetof (type, _klobject_valarray))

#define klobject_attrs_by_class(obj, klclass) (klcast(KlValue*, klcast(char*, (obj)) + (klclass)->attroffset))
#define klobject_attrs(obj)                   ((obj)->attrs)
#define klobject_getlocal(obj, slot)          (klobject_attrs((obj)) + klvalue_getint(&(slot)->value))

#define KLOBJECT_DEFAULT_ATTROFF              (klobject_attrarrayoffset(struct tagKlObjectInstance))



static inline void klobject_getfield(KlObject* object, const KlString* key, KlValue* result);
static inline bool klobject_getmethod(const KlObject* object, const KlString* key, KlValue* result);
static inline KlClass* klobject_class(const KlObject* object);
static inline size_t klobject_size(const KlObject* object);
static inline KlGCObject* klobject_propagate_nomm(const KlObject* object, KlGCObject* gclist);
static inline void klobject_free(KlObject* object, KlMM* klmm);
static inline bool klobject_compatible(const KlObject* object, KlObjectConstructor constructor);

static inline bool klobject_setfield(KlObject* object, const KlString* key, KlValue* value) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) return false;
  if (klclass_slot_is_local(slot)) {
    klvalue_setvalue(klobject_getlocal(object, slot), value);
  } else {
    klvalue_setvalue(&slot->value, value);
  }
  return true;
}

static inline void klobject_getfield(KlObject* object, const KlString* key, KlValue* result) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) {
    klvalue_setnil(result);
  } else if (kl_likely(klclass_slot_is_local(slot))) {
    klvalue_setvalue(result, klobject_getlocal(object, slot));
  } else {
    klvalue_setvalue(result, &slot->value);
  }
}

static inline bool klobject_getmethod(const KlObject* object, const KlString* key, KlValue* result) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) {
    klvalue_setnil(result);
    return false;
  }
  if (klclass_slot_is_local(slot)) {
    klvalue_setvalue(result, klobject_getlocal(object, slot));
    return false;
  } else {
    klvalue_setvalue(result, &slot->value);
    return klclass_slot_is_method(slot);
  }
}

static inline KlClass* klobject_class(const KlObject* object) {
  return object->klclass;
}

static inline size_t klobject_size(const KlObject* object) {
  return object->size;
}

static inline KlGCObject* klobject_propagate_nomm(const KlObject* object, KlGCObject* gclist) {
  klmm_gcobj_mark(klmm_to_gcobj(object->klclass), gclist);
  KlValue* attrs = klobject_attrs(object);
  size_t nlocal = object->klclass->nlocal;
  for (size_t i = 0; i < nlocal; ++i) {
    if (klvalue_collectable(&attrs[i]))
        klmm_gcobj_mark(klvalue_getgcobj(&attrs[i]), gclist);
  }
  return gclist;
}

static inline void klobject_free(KlObject* object, KlMM* klmm) {
  klmm_free(klmm, klmm_to_gcobj(object), klobject_size(object));
}

static inline bool klobject_compatible(const KlObject* object, KlObjectConstructor constructor) {
  return object->klclass->constructor == constructor;
}

#endif
