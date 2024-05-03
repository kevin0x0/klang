#include "include/mm/klmm.h"

#include <stdlib.h>
#include <stdbool.h>

#define klmm_gc_link(list, obj) { obj->next_reachable = list; list = obj; }
#define klmm_gc_pop(list) { list = list->next_reachable; }

static KlGCObject* klmm_gc_start(KlGCObject* root, KlGCObject* list);
static void klmm_gc_markall(KlMM* klmm, KlGCObject* gclist);
static void klmm_gc_dopostproc(KlMM* klmm);
static KlGCObject* klmm_gc_clean(KlMM* klmm, KlGCObject* allgc);


static inline KlGCObject* klmm_gc_nextobj(KlGCObject* obj) {
  return obj->next;
}

static inline KlGCObject* klmm_gc_propagate(KlMM* klmm, KlGCObject* gcobj, KlGCObject* gclist) {
  kl_assert(gcobj->virtualfunc->propagate != NULL, "");
  return gcobj->virtualfunc->propagate(gcobj, klmm, gclist);
}

static inline void klmm_gc_callpostproc(KlMM* klmm, KlGCObject* gcobj) {
  gcobj->virtualfunc->post(gcobj, klmm);
}

static inline bool klmm_gc_cleanable(KlGCObject* gcobj) {
  return !klmm_gcobj_marked(gcobj);
}

static inline void klmm_gc_clean_one(KlMM* klmm, KlGCObject* gcobj) {
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
  KlGCObject* gclist = klmm_gc_start(klmm->root, klmm->allgc);
  klmm_gc_markall(klmm, gclist);
  klmm_gc_dopostproc(klmm);
  klmm->allgc = klmm_gc_clean(klmm, klmm->allgc);
  klmm->limit = klmm->mem_used * 2;
}

KlGCObject* klmm_gc_start(KlGCObject* root, KlGCObject* list) {
  (void)list;
  KlGCObject* gclist = NULL;
  klmm_gc_link(gclist, root);
  root->gc_state |= KLGC_MARKED;
  return gclist;
}

static void klmm_gc_markall(KlMM* klmm, KlGCObject* gclist) {
  while (gclist) {
    KlGCObject* gcobj = gclist;
    klmm_gc_pop(gclist);
    gclist = klmm_gc_propagate(klmm, gcobj, gclist);
  }
}

static void klmm_gc_dopostproc(KlMM* klmm) {
  KlGCObject* obj = klmm->postproclist;
  while (obj) {
    KlGCObject* next = obj->next_post;
    klmm_gc_callpostproc(klmm, obj);
    obj = next;
  }
  klmm->postproclist = NULL;
}

static KlGCObject* klmm_gc_clean(KlMM* klmm, KlGCObject* list) {
  KlGCObject* survivors = NULL;
  for (KlGCObject* gcobj = list; gcobj;) {
    KlGCObject* tmp = klmm_gc_nextobj(gcobj);
    if (klmm_gc_cleanable(gcobj)) {
      klmm_gc_clean_one(klmm, gcobj);
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
    KlGCObject* tmp = klmm_gc_nextobj(gcobj);
    klmm_gc_clean_one(klmm, gcobj);
    gcobj = tmp;
  }
}
