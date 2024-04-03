#include "klang/include/mm/klmm.h"

#include <stdlib.h>
#include <stdbool.h>

#define klmm_gc_link(list, obj) { obj->created.next_reachable = list; list = obj; }
#define klmm_gc_pop(list) { list = list->created.next_reachable; }

static KlGCObject* klmm_gc_start(KlGCObject* root, KlGCObject* list);
static void klmm_gc_mark_accessible(KlGCObject* gclist);
void klmm_gc_clean(KlMM* klmm, KlGCObject* allgc);


static inline KlGCObject* klmm_gc_nextobj(KlGCObject* obj) {
  return obj->next;
}

static inline void klmm_gc_set_stat(KlGCObject* obj, KlGCStat gc_state) {
  obj->created.gc_state = gc_state;
}

static inline KlGCObject* klmm_gc_propagate(KlGCObject* gcobj, KlGCObject* gclist) {
  return gcobj->created.virtualfunc->propagate(gcobj, gclist);
}

static inline bool klmm_gc_propagable(KlGCObject* gcobj) {
  return gcobj->created.virtualfunc->propagate != NULL;
}

static inline bool klmm_gc_cleanable(KlGCObject* gcobj) {
  return gcobj->created.gc_state == KL_GC_INACCESSIBLE;
}

static inline void klmm_gc_clean_one(KlMM* klmm, KlGCObject* gcobj) {
  gcobj->created.virtualfunc->destructor(gcobj, klmm);
  //size_t freesize = gcobj->destructor(gcobj);
  //klmm_free(klmm, gcobj, freesize);
}


void klmm_do_gc(KlMM* klmm) {
  if (!klmm->root) return;
  KlGCObject* gclist = klmm_gc_start(klmm->root, klmm->allgc.next);
  klmm_gc_mark_accessible(gclist);
  klmm_gc_clean(klmm, &klmm->allgc);
  klmm->limit = klmm->mem_used * 2;
}

KlGCObject* klmm_gc_start(KlGCObject* root, KlGCObject* list) {
  (void)list;
  KlGCObject* gclist = NULL;
  klmm_gc_link(gclist, root);
  klmm_gc_set_stat(root, KL_GC_ACCESSIBLE);
  return gclist;
}

void klmm_gc_mark_accessible(KlGCObject* gclist) {
  while (gclist) {
    KlGCObject* gcobj = gclist;
    klmm_gc_pop(gclist);
    if (klmm_gc_propagable(gcobj))
      gclist = klmm_gc_propagate(gcobj, gclist);
  }
}

void klmm_gc_clean(KlMM* klmm, KlGCObject* allgc) {
  allgc->creating.tail->next = NULL;
  KlGCObject* tail = allgc;
  for (KlGCObject* gcobj = allgc->next; gcobj;) {
    KlGCObject* tmp = klmm_gc_nextobj(gcobj);
    if (klmm_gc_cleanable(gcobj)) {
      klmm_gc_clean_one(klmm, gcobj);
    } else {
      tail->next = gcobj;
      tail = gcobj;
      klmm_gc_set_stat(gcobj, KL_GC_INACCESSIBLE);
    }
    gcobj = tmp;
  }
  tail->next = NULL;
  allgc->creating.tail = tail;
}

void klmm_gc_clean_all(KlMM* klmm, KlGCObject* allgc) {
  allgc->creating.tail->next = NULL;
  for (KlGCObject* gcobj = allgc->next; gcobj;) {
    KlGCObject* tmp = klmm_gc_nextobj(gcobj);
    klmm_gc_clean_one(klmm, gcobj);
    gcobj = tmp;
  }
}

// static KlGCObject* klmm_gc_traverse_map(KlGCObject* gclist, KlMap* map) {
//   KlMapIter end = klmap_iter_end(map);
//   KlMapIter begin = klmap_iter_begin(map);
//   for (KlMapIter itr = begin; itr != end; itr = klmap_iter_next(itr)) {
//     KlValue* val = &itr->value;
//     if (collectable(val) && val->value.gcobj->gc_state != KL_GC_REACHABLE) {
//       KlGCObject* gcobj = val->value.gcobj;
//       gcobj->gc_state = KL_GC_REACHABLE;
//       gclink(gclist, gcobj);
//     }
//   }
//   return gclist;
// }
// 
// static KlGCObject* klmm_gc_traverse_array(KlGCObject* gclist, KlArray* array) {
//   KlArrayIter end = klarray_iter_end(array);
//   KlArrayIter begin = klarray_iter_begin(array);
//   for (KlArrayIter itr = begin; itr != end; itr = klarray_iter_next(itr)) {
//     KlValue* val = itr;
//     if (collectable(val) && val->value.gcobj->gc_state != KL_GC_REACHABLE) {
//       KlGCObject* gcobj = val->value.gcobj;
//       gcobj->gc_state = KL_GC_REACHABLE;
//       gclink(gclist, gcobj);
//     }
//   }
//   return gclist;
// }
// 
