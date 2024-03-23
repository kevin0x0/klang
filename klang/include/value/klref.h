#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLREF_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLREF_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klvalue.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct tagKlRefInfo {
  uint8_t index;
  bool in_stack;
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

KlRef* klref_new(KlMM* klmm, KlRef** reflist, KlValue* stkval);
static inline KlRef* klref_get(KlMM* klmm, KlRef** reflist, KlValue* stkval);
static inline void klref_delete(KlMM* klmm, KlRef* ref);

static inline void klref_pin(KlRef* ref);
static inline void klref_unpin(KlMM* klmm, KlRef* ref);

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
    klref_unpin(klmm, reflist);
    reflist = next;
  }
}

static inline bool klref_closed(KlRef* ref) {
  return ref->pval == &ref->closed.val;
}

static inline KlRef* klref_get(KlMM* klmm, KlRef** reflist, KlValue* stkval) {
  KlRef* ref = NULL;
  while ((ref = *reflist)->pval > stkval) {
    reflist = &ref->open.next;
  }
  return ref->pval == stkval ? ref : klref_new(klmm, reflist, stkval);
}

static inline void klref_delete(KlMM* klmm, KlRef* ref) {
  klmm_free(klmm, ref, sizeof (KlRef));
}

static inline void klref_pin(KlRef* ref) {
  ++ref->pincount;
}

static inline void klref_unpin(KlMM* klmm, KlRef* ref) {
  if (--ref->pincount == 0)
    klref_delete(klmm, ref);
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
