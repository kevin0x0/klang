#include "klang/include/value/klclass.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/value/klstring.h"
#include <stddef.h>
#include <string.h>

static KlGCObject* klclass_propagate(KlClass* klclass, KlGCObject* gclist);
static void klclass_delete(KlClass* klclass);

static KlGCVirtualFunc klclass_gcvfunc = { .destructor = (KlGCDestructor)klclass_delete, .propagate = (KlGCProp)klclass_propagate };

static KlClassCeil* klclass_getfreeceil(KlClass* klclass);
static bool klclass_rehash(KlClass* klclass);


KlClass* klclass_create(KlMM* klmm, size_t capacity, size_t attroffset, void* constructor_data, KlObjectConstructor constructor) {
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!klclass)) return NULL;
  klclass->capacity = 1 << capacity;
  klclass->lastfree = klclass->capacity;
  klclass->constructor = constructor ? constructor : klclass_default_constructor;
  klclass->constructor_data = constructor_data;
  klclass->attroffset = attroffset;
  klclass->nlocal = 0;
  klclass->ceils = (KlClassCeil*)klmm_alloc(klmm, klclass->capacity * sizeof (KlClassCeil));
  if (kl_unlikely(!klclass->ceils)) {
    klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }
  for (size_t i = 0; i < klclass->capacity; ++i) {
    klvalue_setnil(&klclass->ceils[i].value);
    klclass->ceils[i].key = NULL;
    klclass->ceils[i].next = NULL;
  }
  klmm_gcobj_enable(klmm, klmm_to_gcobj(klclass), &klclass_gcvfunc);
  return klclass;
}

KlClass* klclass_inherit(KlMM* klmm, KlClass* parent) {
  KlClassCeil* array = (KlClassCeil*)klmm_alloc(klmm, parent->capacity * sizeof (KlClassCeil));
  KlClass* klclass = (KlClass*)klmm_alloc(klmm, sizeof (KlClass));
  if (kl_unlikely(!array || !klclass)) {
    klmm_free(klmm, array, parent->capacity * sizeof (KlClassCeil));
    klmm_free(klmm, klclass, sizeof (KlClass));
    return NULL;
  }

  KlClassCeil* ceils = parent->ceils;
  memcpy(array, ceils, parent->capacity * sizeof (KlClassCeil));
  size_t offset = array - ceils;
  KlClassCeil* end = array + parent->capacity;
  for (KlClassCeil* itr = array; itr != end; ++itr) {
    if (itr->next) itr->next += offset;
  }
  klclass->constructor = parent->constructor;
  klclass->constructor_data = parent->constructor_data;
  klclass->capacity = parent->capacity;
  klclass->lastfree = parent->lastfree;
  klclass->attroffset = parent->attroffset;
  klclass->nlocal = parent->nlocal;
  klclass->ceils = array;
  return klclass;
}

static bool klclass_rehash(KlClass* klclass) {
  KlMM* klmm = klmm_gcobj_getmm(klmm_to_gcobj(klclass));
  KlClassCeil* oldceils = klclass->ceils;
  KlClassCeil* endceils = oldceils + klclass->capacity;
  size_t new_capacity = klclass->capacity * 2;
  klclass->ceils = (KlClassCeil*)klmm_alloc(klmm, new_capacity * sizeof (KlClassCeil)); 
  if (kl_unlikely(!klclass->ceils)) {
    klclass->ceils = oldceils;
    return false;
  }

  for (size_t i = 0; i < new_capacity; ++i) {
    klvalue_setnil(&klclass->ceils[i].value);
    klclass->ceils[i].key = NULL;
    klclass->ceils[i].next = NULL;
  }

  klclass->capacity = new_capacity;
  klclass->lastfree = new_capacity; /* reset lastfree */
  for (KlClassCeil* ceil = oldceils; ceil != endceils; ++ceil) {
    KlClassCeil* inserted = klclass_add(klclass, ceil->key);
    klvalue_setvalue(&inserted->value, &ceil->value);
  }
  klmm_free(klmm, oldceils, (endceils - oldceils) * sizeof (KlClassCeil));
  return true;
}

static KlClassCeil* klclass_getfreeceil(KlClass* klclass) {
  size_t lastfree = klclass->lastfree;
  KlClassCeil* ceils = klclass->ceils;
  while (lastfree--) {
    if (!ceils[lastfree].key) {
      klclass->lastfree = lastfree;
      return &ceils[lastfree];
    }
  }
  return NULL;
}

KlException klclass_set(KlClass* klclass, struct tagKlString* key, KlValue* value) {
  size_t mask = klclass->capacity - 1;
  size_t keyindex = klstring_hash(key) & mask;
  KlClassCeil* ceils = klclass->ceils;
  KlClassCeil* ceil = &ceils[keyindex];
  if (ceil->key) {
    /* find */
    KlClassCeil* findceil = ceil;
    while (findceil) {
      if (findceil->key == key) {
        if (klclass_is_local(findceil)) /* local attribute can not be overridden */
          return KL_E_INVLD;
        klvalue_setvalue(&findceil->value, value);
        return KL_E_NONE;
      }
      findceil = findceil->next;
    }

    /* not found, insert new one */
    KlClassCeil* newceil = klclass_getfreeceil(klclass);
    if (!newceil) { /* no ceil */
      if (kl_unlikely(!klclass_rehash(klclass)))
        return KL_E_OOM;
      KlClassCeil* ceil = klclass_add(klclass, key);
      if (kl_unlikely(!ceil)) return KL_E_OOM;
      klvalue_setvalue(&ceil->value, value);
      return KL_E_NONE;
    }
    size_t ceilindex = klstring_hash(ceil->key) & mask;
    if (ceilindex == keyindex) {
      newceil->key = key;
      newceil->next = ceil->next;
      ceil->next = newceil;
      klvalue_setvalue(&newceil->value, value);
      return KL_E_NONE;
    } else {
      newceil->key = ceil->key;
      klvalue_setvalue(&newceil->value, &ceil->value);
      newceil->next = ceil->next;
      /* correct link */
      KlClassCeil* prevceil = &ceils[ceilindex];
      while (prevceil->next != ceil)
        prevceil = prevceil->next;
      prevceil->next = newceil;

      ceil->key = key;
      ceil->next = NULL;
      klvalue_setvalue(&ceil->value, value);
      return KL_E_NONE;
    }
  } else {
    ceil->key = key;
    ceil->next = NULL;
    klvalue_setvalue(&ceil->value, value);
    return KL_E_NONE;
  }
}

KlClassCeil* klclass_add(KlClass* klclass, KlString* key) {
  size_t mask = klclass->capacity - 1;
  size_t keyindex = klstring_hash(key) & mask;
  KlClassCeil* ceils = klclass->ceils;
  if (ceils[keyindex].key) {
    KlClassCeil* newceil = klclass_getfreeceil(klclass);
    if (!newceil) { /* no ceil */
      if (kl_unlikely(!klclass_rehash(klclass)))
        return NULL;
      return klclass_add(klclass, key);
    }

    size_t ceilindex = klstring_hash(ceils[keyindex].key) & mask;
    if (ceilindex == keyindex) {
      newceil->key = key;
      newceil->next = ceils[keyindex].next;
      ceils[keyindex].next = newceil;
      return newceil;
    } else {
      newceil->key = ceils[keyindex].key;
      klvalue_setvalue(&newceil->value, &ceils[keyindex].value);
      newceil->next = ceils[keyindex].next;
      /* correct link */
      KlClassCeil* prevceil = &ceils[ceilindex];
      while (prevceil->next != &ceils[keyindex])
        prevceil = prevceil->next;
      prevceil->next = newceil;

      ceils[keyindex].key = key;
      ceils[keyindex].next = NULL;
      return &ceils[keyindex];
    }
  } else {
    ceils[keyindex].key = key;
    ceils[keyindex].next = NULL;
    return &ceils[keyindex];
  }
}

KlClassCeil* klclass_find(KlClass* klclass, struct tagKlString* key) {
  size_t keyindex = klstring_hash(key) & (klclass->capacity - 1);
  KlClassCeil* ceil = &klclass->ceils[keyindex];
  if (klclass_isfree(ceil)) return NULL;
  while (ceil) {
    if (ceil->key == key)
      return ceil;
    ceil = ceil->next;
  }
  return NULL;
}

KlClassCeil* klclass_findshared(KlClass* klclass, KlString* key) {
  size_t keyindex = klstring_hash(key) & (klclass->capacity - 1);
  KlClassCeil* ceil = &klclass->ceils[keyindex];
  if (klclass_isfree(ceil)) return NULL;
  while (ceil) {
    if (ceil->key == key) {
      if (klclass_is_local(ceil))
        return NULL;
      return ceil;
    }
    ceil = ceil->next;
  }
  return NULL;
}

static KlGCObject* klclass_propagate(KlClass* klclass, KlGCObject* gclist) {
  KlClassCeil* ceils = klclass->ceils;
  KlClassCeil* end = ceils + klclass->capacity;
  for (KlClassCeil* itr = ceils; itr != end; ++itr) {
    if (itr->key)
      klmm_gcobj_mark_accessible(klmm_to_gcobj(itr->key), gclist);
    if (klvalue_collectable(&itr->value))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(&itr->value), gclist);
  }
  return gclist;
}

static void klclass_delete(KlClass* klclass) {
  KlMM* klmm = klmm_gcobj_getmm(klmm_to_gcobj(klclass));
  klmm_free(klmm, klclass->ceils, klclass->capacity * sizeof (KlClassCeil));
  klmm_free(klmm, klclass, sizeof (KlClass));
}










static void klobject_delete(KlObject* klobject);

static KlGCVirtualFunc klobject_gcvfunc = { .destructor = (KlGCDestructor)klobject_delete, .propagate = (KlGCProp)klobject_propagate };


KlObject* klclass_default_constructor(KlClass* klclass) {
  KlObject* obj = klclass_objalloc(klclass);
  if (!obj) return NULL;
  klmm_gcobj_enable(klclass_getmm(klclass), klmm_to_gcobj(obj), &klobject_gcvfunc);
  return obj;
}

KlObject* klclass_objalloc(KlClass* klclass) {
  size_t nlocal = klclass->nlocal;
  size_t allocsize = klclass->attroffset + sizeof (KlValue) * nlocal;
  KlObject* obj = (KlObject*)klmm_alloc(klclass_getmm(klclass), allocsize);
  if (!obj) return NULL;
  KlValue* attrs = klobject_attrs_by_class(obj, klclass);
  for (size_t i = 0; i < nlocal; ++i)
    klvalue_setnil(&attrs[i]);
  obj->klclass = klclass;
  obj->size = allocsize;
  return obj;
}

KlGCObject* klobject_propagate(KlObject* object, KlGCObject* gclist) {
  klmm_gcobj_mark_accessible(klmm_to_gcobj(object->klclass), gclist);
  KlValue* attrs = klobject_attrs(object);
  size_t nlocal = object->klclass->nlocal;
  for (size_t i = 0; i < nlocal; ++i) {
    if (klvalue_collectable(&attrs[i]))
        klmm_gcobj_mark_accessible(klvalue_getgcobj(&attrs[i]), gclist);
  }
  return gclist;
}

static void klobject_delete(KlObject* object) {
  klobject_free(object);
}
