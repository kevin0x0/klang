#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCLASS_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCLASS_H

#include "klang/include/value/klstring.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klexception.h"

#define KL_CLASS_TAG_SHARED     (0x1)

#define klclass_isfree(slot)    ((slot)->key == NULL)
#define klclass_is_local(slot)  (klvalue_checktype(&(slot)->value, KL_ID))
#define klclass_is_shared(slot) (!klclass_is_local((slot)))


typedef struct tagKlClassSlot KlClassSlot;
typedef struct tagKlClass KlClass;
typedef struct tagKlObject KlObject;
typedef KlException (*KlObjectConstructor)(KlClass* klclass, KlMM* klmm, KlValue* value);

struct tagKlClassSlot {
  KlValue value;
  KlString* key;
  KlClassSlot* next;
};

struct tagKlClass {
  KlGCObject gcbase;
  KlClassSlot* slots;
  KlObjectConstructor constructor;
  void* constructor_data; /* used by constructor */
  size_t capacity;
  size_t lastfree;
  size_t attroffset;
  size_t nlocal;
  bool is_final;
};


struct tagKlObject {
  KlGCObject gcbase;
  KlClass* klclass;
  KlValue* attrs;
  size_t size;
};


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor);
KlClass* klclass_inherit(KlMM* klmm, KlClass* parent);

KlClassSlot* klclass_find(KlClass* klclass, KlString* key);
KlClassSlot* klclass_add(KlClass* klclass, KlMM* klmm, KlString* key);
KlClassSlot* glclass_get(KlClass* klclass, KlMM* klmm, KlString* key);
KlException klclass_newfield(KlClass* klclass, KlMM* klmm, KlString* key, KlValue* value);

KlException klclass_default_constructor(KlClass* klclass, KlMM* klmm, KlValue* value);
static inline void* klclass_constructor_data(KlClass* klclass);
KlObject* klclass_objalloc(KlClass* klclass, KlMM* klmm);
static inline KlException klclass_new_object(KlClass* klclass, KlMM* klmm, KlValue* value);

static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, KlString* key);
static inline KlException klclass_newshared(KlClass* klclass, KlMM* klmm, KlString* key, KlValue* value);


static inline void klclass_final(KlClass* klclass);
static inline bool klclass_isfinal(KlClass* klclass);

static inline void klclass_final(KlClass* klclass) {
  klclass->is_final = true;
}

static inline bool klclass_isfinal(KlClass* klclass) {
  return klclass->is_final;
}

static inline void* klclass_constructor_data(KlClass* klclass) {
  return klclass->constructor_data;
}

static inline KlException klclass_new_object(KlClass* klclass, KlMM* klmm, KlValue* value) {
  return klclass->constructor(klclass, klmm, value);
}

static inline KlException klclass_newlocal(KlClass* klclass, KlMM* klmm, KlString* key) {
  KlValue localid;
  klvalue_setid(&localid, klclass->nlocal++);
  /* if failed, we don't decrease the klclass->nlocal */
  return klclass_newfield(klclass, klmm, key, &localid);
}

static inline KlException klclass_newshared(KlClass* klclass, KlMM* klmm, KlString* key, KlValue* value) {
  return klclass_newfield(klclass, klmm, key, value);
}






#define klobject_attrarrayoffset(type)        (offsetof (type, _klobject_valarray))
#define klobject_tail                         KlValue _klobject_valarray

#define klobject_attrs_by_class(obj, klclass) (klcast(KlValue*, klcast(char*, (obj)) + (klclass)->attroffset))
#define klobject_attrs(obj)                   ((obj)->attrs)

struct tagKlObjectInstance {
  KlObject objbase;
  klobject_tail;
};

#define KLOBJECT_DEFAULT_ATTROFF              (klobject_attrarrayoffset(struct tagKlObjectInstance))



static inline KlValue* klobject_getfield(KlObject* object, KlString* key);
static inline KlClass* klobject_class(KlObject* object);
static inline size_t klobject_size(KlObject* object);
KlGCObject* klobject_propagate(KlObject* object, KlGCObject* gclist);
static inline void klobject_free(KlObject* object, KlMM* klmm);

static inline KlValue* klobject_getfield(KlObject* object, KlString* key) {
  KlClass* klclass = object->klclass;
  KlClassSlot* slot = klclass_find(klclass, key);
  if (kl_unlikely(!slot)) return NULL;
  if (klclass_is_local(slot)) {
    return object->attrs + klvalue_getid(&slot->value);
  } else {
    return &slot->value;
  }
}

static inline KlClass* klobject_class(KlObject* object) {
  return object->klclass;
}

static inline size_t klobject_size(KlObject* object) {
  return object->size;
}

static inline void klobject_free(KlObject* object, KlMM* klmm) {
  klmm_free(klmm, klmm_to_gcobj(object), klobject_size(object));
}

#endif
