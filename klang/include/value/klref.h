#ifndef KEVCC_KLANG_INCLUDE_VALUE_KLREF_H
#define KEVCC_KLANG_INCLUDE_VALUE_KLREF_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klvalue.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct tagKlRefInfo {
  unsigned int index;
  bool in_stack;
} KlRefInfo;

typedef struct tagKlRef KlRef;
struct tagKlRef {
  KlGCObject gcbase;
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

static inline bool klref_closed(KlRef* ref);

KlRef* klref_new(KlMM* klmm, KlRef** reflist, KlValue* stkval);
static KlRef* klref_get(KlMM* klmm, KlRef** reflist, KlValue* stkval);
void klref_close(KlRef** reflist, KlValue* bound, KlMM* klmm);


static inline void klreflist_correct(KlRef* reflist, ptrdiff_t diff);

static inline KlValue* klref_getval(KlRef* ref);



static inline KlRef* klreflist_create(KlMM* klmm) {
  KlRef* ref = (KlRef*)klmm_alloc(klmm, sizeof (KlRef));
  if (kl_unlikely(!ref)) return NULL;
  ref->pval = NULL;
  ref->open.next = NULL;
  return ref;
}

static inline void klreflist_delete(KlRef* reflist, KlMM* klmm) {
  while (reflist) {
    KlRef* tmp = reflist->open.next;
    klmm_free(klmm, reflist, sizeof (KlRef));
    reflist = tmp;
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
