/* klang memory manager and garbage collector */

#ifndef KEVCC_INCLUDE_MM_KLMM_H
#define KEVCC_INCLUDE_MM_KLMM_H

#include "include/lang/kltypes.h"
#include "include/misc/klutils.h"

#include <stdint.h>
#include <stdlib.h>

/* if a gcobject is not in list, then it must be leaf */

#define KLGC_NORM         (0)
#define KLGC_MARKED       (klbit(0))
#define KLGC_ISLEAF       (klbit(1))
#define KLGC_INLIST       (klbit(2))

#define klmm_to_gcobj(obj)            (klcast(KlGCObject*, (obj)))
#define klmm_to_gcobjgeneric(obj)     (klcast(KlGCObjectGeneric*, (obj)))
#define klmm_to_gcobjnotinlist(obj)   (klcast(KlGCObjectNotInList*, (obj)))
#define klmm_gcobj_marked(obj)        (klmm_to_gcobjgeneric((obj))->gc_state & KLGC_MARKED)
#define klmm_gcobj_isleaf(obj)        (klmm_to_gcobjgeneric((obj))->gc_state & KLGC_ISLEAF)
#define klmm_gcobj_isalive(obj)       klmm_gcobj_marked(obj)
#define klmm_gcobj_isdead(obj)        ((klmm_to_gcobjgeneric((obj))->gc_state & KLGC_MARKED) == 0)
#define klmm_gcobj_clearalive(obj)    (klmm_to_gcobjgeneric((obj))->gc_state &= ~KLGC_MARKED)

/* Mark a object accessible and link it to gclist. */
#define klmm_gcobj_mark_accessible(obj, gclist) {               \
  KlGCObject* gcobj = (obj);                                    \
  gcobj->gc_state |= KLGC_MARKED;                               \
  if (!klmm_gcobj_isleaf(gcobj)) {                              \
    gcobj->next_reachable = (gclist);                           \
    (gclist) = gcobj;                                           \
  }                                                             \
}


typedef struct tagKlGCObject KlGCObject;
typedef struct tagKlMM KlMM;

typedef void (*KlGCDestructor)(KlGCObject* gcobj, KlMM* klmm);
typedef struct tagKlGCObject* (*KlGCProp)(KlGCObject* gcobj, KlMM* klmm, KlGCObject* gclist);
typedef void (*KlGCPost)(KlGCObject* gcobj, KlMM* klmm);

typedef unsigned KlGCStat;

typedef struct tagKlGCVirtualFunc {
  KlGCDestructor destructor;
  KlGCProp propagate;
  KlGCPost post;
} KlGCVirtualFunc;



#define KL_DERIVE_FROM_KlGCObjectNotInList(prefix) KlGCStat prefix##gc_state

#define KL_DERIVE_FROM_KlGCObject(prefix)                                                   \
  KL_DERIVE_FROM_KlGCObjectNotInList(prefix);                                               \
  KlGCObject* prefix##next;             /* link all objects */                              \
  KlGCVirtualFunc* prefix##virtualfunc;                                                     \
  union {                                                                                   \
    KlGCObject* prefix##next_reachable; /* link all accessible object in the same level */  \
    KlGCObject* prefix##next_post;      /* next object in postproclist */                   \
  }




/* the pointer of this struct can point to any collectable object */
typedef struct tagKlGCObjectGeneric {
  KL_DERIVE_FROM(KlGCObjectNotInList, );
} KlGCObjectGeneric;

typedef struct tagKlGCObjectNotInList {
  KL_DERIVE_FROM(KlGCObjectNotInList, );
} KlGCObjectNotInList;

struct tagKlGCObject {
  KL_DERIVE_FROM(KlGCObject, );
};


struct tagKlMM {
  KlGCObject* allgc;                        /* top level */
  size_t gcstop_rcs_count;                  /* gc can be stoped recursively */
  KlGCObject* root;
  KlGCObject* postproclist;                 /* objects that need to do something after propagate and before clean */
  size_t mem_used;
  /* if mem_used exceeds this limit, the garbage collection will start.
   * the value of limit will dynamically change. */
  size_t limit; 
};

static inline void klmm_init(KlMM* klmm, size_t limit);
void klmm_destroy(KlMM* klmm);

static inline void klmm_register_root(KlMM* klmm, KlGCObject* root);
static inline KlGCObject* klmm_get_root(KlMM* klmm);


static inline void* klmm_alloc(KlMM* klmm, size_t size);
static inline void* klmm_realloc(KlMM* klmm, void* blk, size_t new_size, size_t old_size);
static inline void klmm_free(KlMM* klmm, void* blk, size_t size);

static inline void klmm_gcobj_postproc(KlMM* klmm, KlGCObject* obj);
static inline void klmm_gcobj_enable(KlMM* klmm, KlGCObject* gcobj, KlGCVirtualFunc* vfunc);
static inline void klmm_gcobj_enable_notinlist(KlMM* klmm, KlGCObjectNotInList* gcobj);
static inline void klmm_stopgc(KlMM* klmm);
static inline void klmm_restartgc(KlMM* klmm);

void klmm_gc_clean_all(KlMM* klmm, KlGCObject* allgc);
void klmm_do_gc(KlMM* klmm);
static inline void klmm_try_gc(KlMM* klmm);



static inline void klmm_init(KlMM* klmm, size_t limit) {
  klmm->allgc = NULL;
  klmm->postproclist = NULL;
  klmm->gcstop_rcs_count = 0;
  klmm->mem_used = 0;
  klmm->limit = limit;
  klmm->root = NULL;
}

static inline void klmm_try_gc(KlMM* klmm) {
  if (klmm->mem_used >= klmm->limit) {
    klmm_do_gc(klmm);
  }
}

static inline void klmm_register_root(KlMM* klmm, KlGCObject* root) {
  klmm->root = root;
}

static inline KlGCObject* klmm_get_root(KlMM* klmm) {
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
  if (blk == NULL) return;
  free(blk);
  klmm->mem_used -= size;
}

static inline void klmm_gcobj_postproc(KlMM* klmm, KlGCObject* obj) {
  obj->next_post = klmm->postproclist;
  klmm->postproclist = obj;
}

static inline void klmm_gcobj_enable(KlMM* klmm, KlGCObject* gcobj, KlGCVirtualFunc* vfunc) {
  gcobj->virtualfunc = vfunc;
  gcobj->gc_state = KLGC_NORM;
  gcobj->next = klmm->allgc;
  klmm->allgc = gcobj;
}

static inline void klmm_gcobj_enable_notinlist(KlMM* klmm, KlGCObjectNotInList* gcobj) {
  kl_unused(klmm);
  gcobj->gc_state = KLGC_NORM | KLGC_ISLEAF;
}

static inline void klmm_stopgc(KlMM* klmm) {
  ++klmm->gcstop_rcs_count;
}

static inline void klmm_restartgc(KlMM* klmm) {
  kl_assert(klmm->gcstop_rcs_count != 0, "");
  --klmm->gcstop_rcs_count;
}

#endif
