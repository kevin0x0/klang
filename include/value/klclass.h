#ifndef _KLANG_INCLUDE_VALUE_KLCLASS_H_
#define _KLANG_INCLUDE_VALUE_KLCLASS_H_

#include "include/lang/kltypes.h"
#include "include/misc/klutils.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

#define KLCLASS_TAG_NORMAL      klcast(KlUnsigned, 0)
#define KLCLASS_TAG_METHOD      klcast(KlUnsigned, klbit(0))

#define klclass_isfree(slot)    ((slot)->key == NULL)
#define klclass_is_local(slot)  (klvalue_checktype(&(slot)->value, KL_UINT))
#define klclass_is_shared(slot) (!klclass_is_local((slot)))


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
  size_t capacity;
  size_t lastfree;
  size_t attroffset;
  KlUnsigned nlocal;
  bool is_final;
};

#define KL_DERIVE_FROM_KlObject(prefix)         \
  KL_DERIVE_FROM_KlGCObject(prefix##_gcbase_);  \
  KlClass* prefix##klclass;                     \
  KlValue* prefix##attrs;                       \
  size_t prefix##size

#define KLOBJECT_TAIL                         KlValue _klobject_valarray

struct tagKlObject {
  KL_DERIVE_FROM(KlObject, );
  KLOBJECT_TAIL;
};


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor);
KlClass* klclass_inherit(KlMM* klmm, const KlClass* parent);

static inline KlClassSlot* klclass_find(const KlClass* klclass, const KlString* key);
KlClassSlot* klclass_add(KlClass* klclass, KlMM* klmm, const KlString* key);
KlException klclass_newfield(KlClass* klclass, KlMM* klmm, const KlString* key, const KlValue* value);

KlException klclass_default_constructor(KlClass* klclass, KlMM* klmm, KlValue* value);
static inline void* klclass_constructor_data(KlClass* klclass);
KlObject* klclass_objalloc(KlClass* klclass, KlMM* klmm);
static inline KlException klclass_new_object(KlClass* klclass, KlMM* klmm, KlValue* value);

static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, const KlString* key);
static inline KlException klclass_newshared_normal(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value);
static inline KlException klclass_newshared_method(KlClass* klclass, KlMM* klmm, const KlString* key, KlValue* value);


static inline void klclass_final(KlClass* klclass);
static inline bool klclass_isfinal(KlClass* klclass);
static inline KlUnsigned klclass_nlocal(KlClass* klclass);

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

static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, const KlString* key) {
  KlValue localid;
  klvalue_setuint(&localid, klclass->nlocal++);
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
  size_t keyindex = klstring_hash(key) & (klclass->capacity - 1);
  KlClassSlot* slot = &klclass->slots[keyindex];
  while (slot) {
    if (slot->key == key)
      return slot;
    slot = slot->next;
  }
  return NULL;
}





#define klobject_attrarrayoffset(type)        (offsetof (type, _klobject_valarray))

#define klobject_attrs_by_class(obj, klclass) (klcast(KlValue*, klcast(char*, (obj)) + (klclass)->attroffset))
#define klobject_attrs(obj)                   ((obj)->attrs)

#define KLOBJECT_DEFAULT_ATTROFF              (klobject_attrarrayoffset(struct tagKlObject))



static inline KlValue* klobject_getfield(const KlObject* object, const KlString* key);
static inline bool klobject_getmethod(const KlObject* object, const KlString* key, KlValue* result);
static inline KlClass* klobject_class(const KlObject* object);
static inline size_t klobject_size(const KlObject* object);
static inline KlGCObject* klobject_propagate_nomm(const KlObject* object, KlGCObject* gclist);
static inline void klobject_free(KlObject* object, KlMM* klmm);
static inline bool klobject_compatible(const KlObject* object, KlObjectConstructor constructor);

static inline KlValue* klobject_getfield(const KlObject* object, const KlString* key) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) return NULL;
  if (klclass_is_local(slot)) {
    return klobject_attrs(object) + klvalue_getuint(&slot->value);
  } else {
    return &slot->value;
  }
}

static inline bool klobject_setfield(KlObject* object, KlString* key, KlValue* value) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) return false;
  if (klclass_is_local(slot)) {
    klvalue_setvalue(klobject_attrs(object) + klvalue_getuint(&slot->value), value);
  } else {
    klvalue_setvalue(&slot->value, value);
  }
  return true;
}

static inline void klobject_getfieldset(KlObject* object, KlString* key, KlValue* result) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) {
    klvalue_setnil(result);
  } else if (klclass_is_local(slot)) {
    klvalue_setvalue(result, klobject_attrs(object) + klvalue_getuint(&slot->value));
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
  if (klclass_is_local(slot)) {
    klvalue_setvalue(result, klobject_attrs(object) + klvalue_getuint(&slot->value));
    return false;
  } else {
    klvalue_setvalue(result, &slot->value);
    return klvalue_gettag(&slot->value) & KLCLASS_TAG_METHOD;
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
