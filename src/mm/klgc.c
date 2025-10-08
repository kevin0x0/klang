#include "include/mm/klmm.h"

#include <stdlib.h>
#include <stdbool.h>

#define klmm_gc_link(list, obj) { obj->next_reachable = list; list = obj; }
#define klmm_gc_pop(list) { list = list->next_reachable; }

static KlGCObject* gc_start(KlGCObject* root, KlGCObject* list);
static void markall(KlMM* klmm, KlGCObject* gclist);
static KlGCObject* doafter(KlMM* klmm, KlGCObject* afterlist);
static KlGCObject* gc_clean(KlMM* klmm, KlGCObject* allgc);


static inline KlGCObject* nextgcobj(KlGCObject* obj) {
  return obj->next;
}

static inline KlGCObject* propagate(KlMM* klmm, KlGCObject* gcobj, KlGCObject* gclist) {
  kl_assert(gcobj->virtualfunc->propagate != NULL, "");
  return gcobj->virtualfunc->propagate(gcobj, klmm, gclist);
}

static inline void call_after_callback(KlMM* klmm, KlGCObject* gcobj) {
  gcobj->virtualfunc->after(gcobj, klmm);
}

static inline bool cleanable(KlGCObject* gcobj) {
  return !klmm_gcobj_marked(gcobj);
}

static inline void clean_one(KlMM* klmm, KlGCObject* gcobj) {
  gcobj->virtualfunc->destructor(gcobj, klmm);
  //size_t freesize = gcobj->destructor(gcobj);
  //klmm_free(klmm, gcobj, freesize);
}

void klmm_do_gc(KlMM* klmm) {
  if (!klmm->root) return;
  if (klmm->gcstop_rcs_count) {
    klmm->limit = klmm->mem_used + klmm->mem_used / 2;
    return;
  }
  KlGCObject* gclist = gc_start(klmm->root, klmm->allgc);
  markall(klmm, gclist);
  klmm->aftermark = doafter(klmm, klmm->aftermark);
  klmm->allgc = gc_clean(klmm, klmm->allgc);
  klmm->aftersweep = doafter(klmm, klmm->aftersweep);
  klmm->limit = klmm->mem_used * 2;
}

KlGCObject* gc_start(KlGCObject* root, KlGCObject* list) {
  (void)list;
  KlGCObject* gclist = NULL;
  klmm_gc_link(gclist, root);
  root->base.gc_state |= KLGC_MARKED;
  return gclist;
}

static void markall(KlMM* klmm, KlGCObject* gclist) {
  while (gclist) {
    KlGCObject* gcobj = gclist;
    klmm_gc_pop(gclist);
    gclist = propagate(klmm, gcobj, gclist);
  }
}

static KlGCObject* doafter(KlMM* klmm, KlGCObject* afterlist) {
  KlGCObject* obj = afterlist;
  while (obj) {
    KlGCObject* next = obj->next_after;
    call_after_callback(klmm, obj);
    obj = next;
  }
  return NULL;
}

static KlGCObject* gc_clean(KlMM* klmm, KlGCObject* list) {
  KlGCObject* survivors = NULL;
  for (KlGCObject* gcobj = list; gcobj;) {
    KlGCObject* tmp = nextgcobj(gcobj);
    if (cleanable(gcobj)) {
      clean_one(klmm, gcobj);
    } else {
      klmm_gcobj_clearalive(gcobj);
      gcobj->next = survivors;
      survivors = gcobj;
    }
    gcobj = tmp;
  }
  return survivors;
}

void klmm_gc_clean_all(KlMM* klmm, KlGCObject* list) {
  for (KlGCObject* gcobj = list; gcobj;) {
    KlGCObject* tmp = nextgcobj(gcobj);
    clean_one(klmm, gcobj);
    gcobj = tmp;
  }
}
