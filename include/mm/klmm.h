/* klang memory manager and garbage collector */

#ifndef _INCLUDE_MM_KLMM_H_
#define _INCLUDE_MM_KLMM_H_

#include "include/common/kltypes.h"
#include "include/misc/klutils.h"

#include <stdlib.h>

/* if a gcobject is not in list(is a delegated gcobject), then it must be leaf */

#define KLGC_NORM         (klcast(KlGCStat, 0))
#define KLGC_MARKED       (klcast(KlGCStat, klbit(0)))
#define KLGC_ISLEAF       (klcast(KlGCStat, klbit(1)))
#define KLGC_INLIST       (klcast(KlGCStat, klbit(2)))

#define klmm_to_gcobj(obj)            (klcast(KlGCObject*, (obj)))
#define klmm_to_gcobjgeneric(obj)     (klcast(KlGCObjectGeneric*, (obj)))
#define klmm_to_gcobjdelegate(obj)    (klcast(KlGCObjectDelegate*, (obj)))
#define klmm_gcobj_marked(obj)        (klmm_to_gcobjgeneric((obj))->gc_state & KLGC_MARKED)
#define klmm_gcobj_isleaf(obj)        (klmm_to_gcobjgeneric((obj))->gc_state & KLGC_ISLEAF)
#define klmm_gcobj_isalive(obj)       klmm_gcobj_marked(obj)
#define klmm_gcobj_isdead(obj)        ((klmm_to_gcobjgeneric((obj))->gc_state & KLGC_MARKED) == 0)
#define klmm_gcobj_clearalive(obj)    (klmm_to_gcobjgeneric((obj))->gc_state &= (~KLGC_MARKED))
#define klmm_gcobj_shouldprop(obj)    (!(klmm_to_gcobjgeneric((obj))->gc_state & (KLGC_MARKED | KLGC_ISLEAF)))

/* Mark a object accessible and link it to gclist. */
#define klmm_gcobj_mark(obj, gclist) {                          \
  KlGCObject* gcobj = (obj);                                    \
  if (klmm_gcobj_shouldprop(gcobj)) {                           \
    gcobj->next_reachable = (gclist);                           \
    (gclist) = gcobj;                                           \
  }                                                             \
  klmm_to_gcobjgeneric(gcobj)->gc_state |= KLGC_MARKED;         \
}


typedef struct tagKlGCObject KlGCObject;
typedef struct tagKlMM KlMM;

typedef void (*KlGCDestructor)(KlGCObject* gcobj, KlMM* klmm);
typedef struct tagKlGCObject* (*KlGCProp)(KlGCObject* gcobj, KlMM* klmm, KlGCObject* gclist);
typedef void (*KlGCAfter)(KlGCObject* gcobj, KlMM* klmm);

typedef KlUnsigned KlGCStat;

typedef struct tagKlGCVirtualFunc {
  KlGCDestructor destructor;
  KlGCProp propagate;
  KlGCAfter after;
} KlGCVirtualFunc;


/* the pointer of this struct can point to any collectable object */
typedef struct tagKlGCObjectDelegate KlGCObjectGeneric;

typedef struct tagKlGCObjectDelegate {
  KlGCStat gc_state;
} KlGCObjectDelegate;

struct tagKlGCObject {
  KL_DERIVE_FROM(KlGCObjectDelegate, base);
  KlGCObject* next;             /* link all objects */
  const KlGCVirtualFunc* virtualfunc;
  union {
    KlGCObject* next_reachable; /* link all accessible object in the same level */
    KlGCObject* next_after;     /* next object in after list */
  };
};


struct tagKlMM {
  KlGCObject* allgc;                        /* gc objects */
  KlGCObject* root;
  KlGCObject* aftermark;                    /* objects that need to do something after propagate mark */
  KlGCObject* aftersweep;                   /* objects that need to do something after sweeping */
  size_t mem_used;
  /* if mem_used exceeds this limit, the garbage collection will start.
   * the value of limit will dynamically change. */
  size_t limit; 
  unsigned gcstop_rcs_count;                /* gc can be stopped recursively */
};

static inline void klmm_init(KlMM* klmm, size_t limit);
void klmm_destroy(KlMM* klmm);

static inline void klmm_register_root(KlMM* klmm, KlGCObject* root);
static inline KlGCObject* klmm_get_root(const KlMM* klmm);


static inline void* klmm_alloc(KlMM* klmm, size_t size);
static inline void* klmm_realloc(KlMM* klmm, void* blk, size_t new_size, size_t old_size);
static inline void klmm_free(KlMM* klmm, void* blk, size_t size);

static inline void klmm_gcobj_aftermark(KlMM* klmm, KlGCObject* obj);
static inline void klmm_gcobj_aftersweep(KlMM* klmm, KlGCObject* obj);
static inline void klmm_gcobj_enable(KlMM* klmm, KlGCObject* gcobj, const KlGCVirtualFunc* vfunc);
static inline void klmm_gcobj_enable_delegate(KlMM* klmm, KlGCObjectDelegate* gcobj);
static inline void klmm_stopgc(KlMM* klmm);
static inline void klmm_restartgc(KlMM* klmm);

void klmm_gc_clean_all(KlMM* klmm, KlGCObject* allgc);
void klmm_do_gc(KlMM* klmm);
static inline void klmm_try_gc(KlMM* klmm);



static inline void klmm_init(KlMM* klmm, size_t limit) {
  klmm->allgc = NULL;
  klmm->aftermark = NULL;
  klmm->aftersweep = NULL;
  klmm->gcstop_rcs_count = 0;
  klmm->mem_used = 0;
  klmm->limit = limit;
  klmm->root = NULL;
}

static inline size_t klmm_memory_usage(KlMM *klmm) {
  return klmm->mem_used;
}

static inline size_t klmm_memory_limit(KlMM *klmm) {
  return klmm->limit;
}

static inline void klmm_try_gc(KlMM* klmm) {
#ifdef KLANG_DEBUG_ALWAYS_GC
    klmm_do_gc(klmm);
#else
  if (klmm->mem_used >= klmm->limit) {
    klmm_do_gc(klmm);
  }
#endif
}

static inline void klmm_register_root(KlMM* klmm, KlGCObject* root) {
  klmm->root = root;
}

static inline KlGCObject* klmm_get_root(const KlMM* klmm) {
  return klmm->root;
}

static inline void* klmm_alloc(KlMM* klmm, size_t size) {
  klmm_try_gc(klmm);

  void* blk = malloc(size);
  if (blk) {
    klmm->mem_used += size;
    return blk;
  }
  /* second try */
  klmm_do_gc(klmm);
  blk = malloc(size);
  if (blk)
    klmm->mem_used += size;
  return blk;
}

static inline void* klmm_realloc(KlMM* klmm, void* blk, size_t new_size, size_t old_size) {
  klmm_try_gc(klmm);

  void* new_blk = realloc(blk, new_size);
  if (new_blk) {
    klmm->mem_used += new_size - old_size;
    return new_blk;
  }
  /* second try */
  klmm_do_gc(klmm);
  new_blk = realloc(blk, new_size);
  if (new_blk)
    klmm->mem_used += new_size - old_size;
  return new_blk;
}

static inline void klmm_free(KlMM* klmm, void* blk, size_t size) {
  kl_assert(blk != NULL, "should not free a NULL pointer");
  free(blk);
  klmm->mem_used -= size;
}

static inline void klmm_gcobj_aftermark(KlMM* klmm, KlGCObject* obj) {
  obj->next_after = klmm->aftermark;
  klmm->aftermark = obj;
}

static inline void klmm_gcobj_aftersweep(KlMM* klmm, KlGCObject* obj) {
  obj->next_after = klmm->aftersweep;
  klmm->aftersweep = obj;
}

static inline void klmm_gcobj_enable(KlMM* klmm, KlGCObject* gcobj, const KlGCVirtualFunc* vfunc) {
  gcobj->virtualfunc = vfunc;
  gcobj->base.gc_state = KLGC_INLIST;
  gcobj->next = klmm->allgc;
  klmm->allgc = gcobj;
}

static inline void klmm_gcobj_enable_delegate(KlMM* klmm, KlGCObjectDelegate* gcobj) {
  kl_unused(klmm);
  gcobj->gc_state = KLGC_ISLEAF;
}

static inline void klmm_stopgc(KlMM* klmm) {
  ++klmm->gcstop_rcs_count;
}

static inline void klmm_restartgc(KlMM* klmm) {
  kl_assert(klmm->gcstop_rcs_count != 0, "");
  --klmm->gcstop_rcs_count;
}

#endif
