/* kl memory manager */

#ifndef KEVCC_INCLUDE_MM_MM_H
#define KEVCC_INCLUDE_MM_MM_H
#include "klang/include/value/value.h"


typedef struct tagKlMM {
  KlValue* valuelist;
  size_t listlen;
  /* if listlen exceeds this limit, the garbage collection will start
   * the value of limit will dynamically change. */
  size_t limit; 
} KlMM;

static inline void klmm_init(KlMM* klmm);
void klmm_destroy(KlMM* klmm);

static inline KlValue* klmm_alloc_map(KlMM* klmm);
static inline KlValue* klmm_alloc_array(KlMM* klmm);
static inline KlValue* klmm_alloc_int(KlMM* klmm, KlInt val);
static inline KlValue* klmm_alloc_string(KlMM* klmm, KString* str);
static inline KlValue* klmm_alloc_copy(KlMM* klmm, KlValue* value);

KlValue* klmm_do_gc(KlMM* klmm);

static inline void klmm_init(KlMM* klmm) {
  klmm->valuelist = NULL;
  klmm->listlen = 0;
}

static inline KlValue* klmm_alloc_map(KlMM* klmm) {
}

static inline KlValue* klmm_alloc_array(KlMM* klmm) {
}

static inline KlValue* klmm_alloc_int(KlMM* klmm, KlInt val) {
}

static inline KlValue* klmm_alloc_string(KlMM* klmm, KString* str) {
}

static inline KlValue* klmm_alloc_copy(KlMM* klmm, KlValue* value) {
}


#endif
