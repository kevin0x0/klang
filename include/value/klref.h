#ifndef _KLANG_INCLUDE_VALUE_KLREF_H_
#define _KLANG_INCLUDE_VALUE_KLREF_H_

#include "include/mm/klmm.h"
#include "include/value/klvalue.h"
#include <stddef.h>

typedef struct tagKlRefInfo {
  KlUByte index;
  bool on_stack;
} KlRefInfo;

typedef struct tagKlRef KlRef;
struct tagKlRef {
  size_t pincount;
  KlValue* pval;  /* point to the value */
  union {
    struct {
      KlRef* next;  /* link all open references */
    } open;
    struct {
      KlValue val;  /* when closed, the pval points to this */
    } closed;
  };
};


static inline KlRef* klreflist_create(KlMM* klmm);
static inline void klreflist_delete(KlRef* reflist, KlMM* klmm);
void klreflist_close(KlRef** reflist, KlValue* bound, KlMM* klmm);

static inline bool klref_closed(KlRef* ref);

KlRef* klref_new(KlRef** reflist, KlMM* klmm, KlValue* stkval);
static inline KlRef* klref_get(KlRef** reflist, KlMM* klmm, KlValue* stkval);
static inline void klref_delete(KlRef* ref, KlMM* klmm);

static inline void klref_pin(KlRef* ref);
static inline void klref_unpin(KlRef* ref, KlMM* klmm);

static inline KlGCObject* klref_propagate(KlRef* ref, KlGCObject* gclist);

static inline void klreflist_correct(KlRef* reflist, ptrdiff_t diff);

static inline KlValue* klref_getval(KlRef* ref);



static inline KlRef* klreflist_create(KlMM* klmm) {
  KlRef* ref = (KlRef*)klmm_alloc(klmm, sizeof (KlRef));
  if (kl_unlikely(!ref)) return NULL;
  ref->pval = NULL;
  ref->open.next = NULL;
  ref->pincount = 0;
  klref_pin(ref);
  return ref;
}

static inline void klreflist_delete(KlRef* reflist, KlMM* klmm) {
  while (reflist) {
    KlRef* next = reflist->open.next;
    klref_unpin(reflist, klmm);
    reflist = next;
  }
}

static inline bool klref_closed(KlRef* ref) {
  return ref->pval == &ref->closed.val;
}

static inline KlRef* klref_get(KlRef** reflist, KlMM* klmm, KlValue* stkval) {
  KlRef* ref = NULL;
  while ((ref = *reflist)->pval > stkval) {
    reflist = &ref->open.next;
  }
  return ref->pval == stkval ? ref : klref_new(reflist, klmm, stkval);
}

static inline void klref_delete(KlRef* ref, KlMM* klmm) {
  klmm_free(klmm, ref, sizeof (KlRef));
}

static inline void klref_pin(KlRef* ref) {
  ++ref->pincount;
}

static inline void klref_unpin(KlRef* ref, KlMM* klmm) {
  if (--ref->pincount == 0)
    klref_delete(ref, klmm);
}

static inline KlGCObject* klref_propagate(KlRef* ref, KlGCObject* gclist) {
  if (klvalue_collectable(&ref->closed.val))
    klmm_gcobj_mark_accessible(klvalue_getgcobj(&ref->closed.val), gclist);
  return gclist;
}

static inline KlValue* klref_getval(KlRef* ref) {
  return ref->pval;
}

static inline void klreflist_correct(KlRef* reflist, ptrdiff_t diff) {
  if (diff == 0) return;  /* it is possible because the stack reallocation uses realloc() */
  KlRef* ref = reflist;
  while (ref->pval) {
    ref->pval += diff;
    ref = ref->open.next;
  }
}

#endif
