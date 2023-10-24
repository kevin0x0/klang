/* kl memory manager */

#ifndef KEVCC_INCLUDE_KLMM_H
#define KEVCC_INCLUDE_KLMM_H

#include "klang/include/klgc.h"
#include "klang/include/value/value.h"

#define KLMM_LIMIT  (1024)


typedef struct tagKlMM {
  KlValue* valuelist;
  /* root value, gc starts with this.
   * if NULL, the gc will not start. 
   * to enable gc, root must be specified. */
  KlValue* root;
  size_t listlen;
  /* if listlen exceeds this limit, the garbage collection will start.
   * the value of limit will dynamically change. */
  size_t limit; 
} KlMM;

static inline void klmm_init(KlMM* klmm, size_t limit);
void klmm_destroy(KlMM* klmm);

static inline void klmm_register_root(KlMM* klmm, KlValue* root);

static inline KlValue* klmm_alloc_map(KlMM* klmm);
static inline KlValue* klmm_alloc_array(KlMM* klmm);
static inline KlValue* klmm_alloc_int(KlMM* klmm, KlInt val);
static inline KlValue* klmm_alloc_string(KlMM* klmm, KString* str);
static inline KlValue* klmm_alloc_copy(KlMM* klmm, KlValue* value);

static inline void klmm_try_gc(KlMM* klmm);
static inline void klmm_do_gc(KlMM* klmm);

static inline void klmm_init(KlMM* klmm, size_t limit) {
  klmm->valuelist = NULL;
  klmm->listlen = 0;
  klmm->limit = limit;
  klmm->root = NULL;
}

static inline void klmm_try_gc(KlMM* klmm) {
  if (klmm->listlen >= klmm->limit)
    klmm_do_gc(klmm);
}

static inline KlValue* klmm_alloc_map(KlMM* klmm) {
  klmm_try_gc(klmm);
  KlValue* value = klvalue_create_map();
  if (!value) return NULL;
  ++klmm->listlen;
  value->next = klmm->valuelist;
  klmm->valuelist = value;
  return value;
}

static inline KlValue* klmm_alloc_array(KlMM* klmm) {
  klmm_try_gc(klmm);
  KlValue* value = klvalue_create_array();
  if (!value) return NULL;
  ++klmm->listlen;
  value->next = klmm->valuelist;
  klmm->valuelist = value;
  return value;
}

static inline KlValue* klmm_alloc_int(KlMM* klmm, KlInt val) {
  klmm_try_gc(klmm);
  KlValue* value = klvalue_create_int(val);
  if (!value) return NULL;
  ++klmm->listlen;
  value->next = klmm->valuelist;
  klmm->valuelist = value;
  return value;
}

static inline KlValue* klmm_alloc_string(KlMM* klmm, KString* str) {
  klmm_try_gc(klmm);
  KlValue* value = klvalue_create_string(str);
  if (!value) return NULL;
  ++klmm->listlen;
  value->next = klmm->valuelist;
  klmm->valuelist = value;
  return value;
}


static inline KlValue* klmm_alloc_copy(KlMM* klmm, KlValue* value) {
  klmm_try_gc(klmm);
  KlValue* copy = klvalue_create_shallow_copy(value);
  if (!copy) return NULL;
  ++klmm->listlen;
  copy->next = klmm->valuelist;
  klmm->valuelist = copy;
  return value;
}

static inline void klmm_do_gc(KlMM* klmm) {
  if (!klmm->root) return;
  KlValue* gclist = klgc_start(klmm->root, klmm->valuelist);
  klgc_mark_reachable(gclist);
  klmm->valuelist = klgc_clean(klmm->valuelist, &klmm->listlen);
  klmm->limit = klmm->listlen * 2;
}

static inline void klmm_register_root(KlMM* klmm, KlValue* root) {
  klmm->root = root;
}

#endif
