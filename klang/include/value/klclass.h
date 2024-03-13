#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLCLASS_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLCLASS_H

#include "klang/include/value/klstring.h"
#include "klang/include/value/klvalue.h"
#include <stdbool.h>

#define KL_CLASS_TAG_SHARED     (0x1)

#define klclass_isfree(ceil)    ((ceil)->key == NULL)
#define klclass_is_local(ceil)  (klvalue_checktype(&(ceil)->value, KL_ID))
#define klclass_is_shared(ceil) (!klclass_is_local((ceil)))


typedef struct tagKlClassCeil KlClassCeil;
typedef struct tagKlClass KlClass;
typedef struct tagKlObject KlObject;
typedef KlObject* (*KlObjectConstructor)(KlClass* klclass);

struct tagKlClassCeil {
  KlValue value;
  KlString* key;
  KlClassCeil* next;
};

struct tagKlClass {
  KlGCObject gcbase;
  KlClassCeil* ceils;
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
  size_t size;
};


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor);
KlClass* klclass_inherit(KlMM* klmm, KlClass* parent);

KlClassCeil* klclass_find(KlClass* klclass, KlString* key);
KlClassCeil* klclass_add(KlClass* klclass, KlString* key);
KlException klclass_set(KlClass* klclass, KlString* key, KlValue* value);
KlClassCeil* klclass_findshared(KlClass* klclass, KlString* key);

KlObject* klclass_default_constructor(KlClass* klclass);
static inline void* klclass_constructor_data(KlClass* klclass);
KlObject* klclass_objalloc(KlClass* klclass, KlMM* klmm);
static inline KlObject* klclass_new_object(KlClass* klclass);

static inline KlException klclass_newlocal(KlClass* klclass, KlString* key);
static inline KlException klclass_newshared(KlClass* klclass, KlString* key, KlValue* value);


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

static inline KlObject* klclass_new_object(KlClass* klclass) {
  return klclass->constructor(klclass);
}

static inline KlException klclass_newlocal(KlClass* klclass, KlString* key) {
  KlValue tmpval;
  klvalue_setid(&tmpval, klclass->nlocal++);
  KlException exception = klclass_set(klclass, key, &tmpval);
  if (kl_unlikely(exception)) --klclass->nlocal;
  return exception;
}

static inline KlException klclass_newshared(KlClass* klclass, KlString* key, KlValue* value) {
  return klclass_set(klclass, key, value);
}






#define klobject_attrarrayoffset(type)        (offsetof (type, _klobject_valarray))
#define klobject_tail                         KlValue _klobject_valarray

#define klobject_attrs_by_class(obj, klclass) (klcast(KlValue*, klcast(char*, (obj)) + (klclass)->attroffset))
#define klobject_attrs(obj)                   (klobject_attrs_by_class((obj), (obj)->klclass))

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
  KlClassCeil* ceil = klclass_find(klclass, key);
  if (kl_unlikely(!ceil)) return NULL;
  if (klclass_is_local(ceil)) {
    return klobject_attrs_by_class(object, klclass) + klvalue_getid(&ceil->value);
  } else {
    return &ceil->value;
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
