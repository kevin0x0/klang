#ifndef KEVCC_KLANG_INCLUDE_VM_KLSTACK_H
#define KEVCC_KLANG_INCLUDE_VM_KLSTACK_H

#include "klang/include/mm/klmm.h"
#include "klang/include/value/klvalue.h"

#define klstack_pushobj(stack, obj, type)   klstack_pushgcobj((stack), klmm_to_gcobj((obj)), (type))

typedef struct tagKlStack {
  KlValue* curr;
  KlValue* array;
  KlValue* end;
} KlStack;

bool klstack_init(KlStack* stack, KlMM* klmm);
static inline void klstack_destroy(KlStack* stack, KlMM* klmm);

static inline KlGCObject* klstack_propagate(KlStack* stack, KlGCObject* gclist);

static inline bool klstack_instack(KlStack* stack, KlValue* val);

static inline size_t klstack_size(KlStack* stack);
static inline size_t klstack_residual(KlStack* stack);
static inline size_t klstack_capacity(KlStack* stack);

static inline KlValue* klstack_raw(KlStack* stack);

static inline bool klstack_check_index(KlStack* stack, int index);
static inline bool klstack_check_index_base(KlStack* stack, size_t index);

static inline KlValue* klstack_end(KlStack* stack);
static inline KlValue* klstack_top(KlStack* stack);
static inline void klstack_set_top(KlStack* stack, KlValue* top);
static inline void klstack_move_top(KlStack* stack, int offset);

/* no protect */
static inline void klstack_pushnil(KlStack* stack, size_t count);
static inline void klstack_pushint(KlStack* stack, KlInt val);
static inline void klstack_pushfloat(KlStack* stack, KlFloat val);
static inline void klstack_pushbool(KlStack* stack, KlBool val);
static inline void klstack_pushcfunc(KlStack* stack, KlCFunction* cfunc);
static inline void klstack_pushgcobj(KlStack* stack, KlGCObject* gcobj, KlType type);
static inline void klstack_pushvalue(KlStack* stack, KlValue* val);

static inline bool klstack_expand_if_full(KlStack* stack, KlMM* klmm);

bool klstack_expand(KlStack* stack, KlMM* klmm);


static inline void klstack_destroy(KlStack* stack, KlMM* klmm) {
  klmm_free(klmm, stack->array, sizeof (KlValue) * klstack_capacity(stack));
  stack->curr = NULL;
  stack->array = NULL;
  stack->end = NULL;
}

static inline KlGCObject* klstack_propagate(KlStack* stack, KlGCObject* gclist) {
  KlValue* top = stack->curr;
  for (KlValue* p = stack->array; p != top; ++p) {
    if (klvalue_collectable(p))
      klmm_gcobj_mark_accessible(klvalue_getgcobj(p), gclist);
  }
  KlValue* end = stack->end;
  /* ensure the entire stack has no dead gcobject */
  for(KlValue* p = top; p < end; ++p)
    klvalue_setnil(p);
  return gclist;
}

static inline bool klstack_instack(KlStack* stack, KlValue* val) {
  return val < stack->curr && val >= stack->array;
}

static inline size_t klstack_size(KlStack* stack) {
  return stack->curr - stack->array;
}

static inline size_t klstack_residual(KlStack* stack) {
  return stack->end - stack->curr;
}

static inline size_t klstack_capacity(KlStack* stack) {
  return stack->end - stack->array;
}

static inline KlValue* klstack_raw(KlStack* stack) {
  return stack->array;
}

static inline bool klstack_check_index(KlStack* stack, int index) {
  return stack->curr + index >= stack->array && index < 0;
}

static inline bool klstack_check_index_base(KlStack* stack, size_t index) {
  return stack->curr > stack->array + index;
}

static inline KlValue* klstack_end(KlStack* stack) {
  return stack->end;
}

static inline KlValue* klstack_top(KlStack* stack) {
  return stack->curr;
}

static inline void klstack_set_top(KlStack* stack, KlValue* top) {
  stack->curr = top;
}

static inline void klstack_move_top(KlStack* stack, int offset) {
  stack->curr += offset;
}

static inline void klstack_pushnil(KlStack* stack, size_t count) {
  KlValue* p = stack->curr;
  while (count--) {
    klvalue_setnil(p++);
  }
  stack->curr = p;
}

static inline void klstack_pushint(KlStack* stack, KlInt val) {
  klvalue_setint(stack->curr++, val);
}

static inline void klstack_pushfloat(KlStack* stack, KlFloat val) {
  klvalue_setfloat(stack->curr++, val);
}

static inline void klstack_pushbool(KlStack* stack, KlBool val) {
  klvalue_setbool(stack->curr++, val);
}

static inline void klstack_pushcfunc(KlStack* stack, KlCFunction* cfunc) {
  klvalue_setcfunc(stack->curr++, cfunc);
}

static inline void klstack_pushgcobj(KlStack* stack, KlGCObject* gcobj, KlType type) {
  klvalue_setgcobj(stack->curr++, gcobj, type);
}

static inline void klstack_pushvalue(KlStack* stack, KlValue* val) {
  klvalue_setvalue(stack->curr++, val);
}

static inline bool klstack_expand_if_full(KlStack* stack, KlMM* klmm) {
  if (kl_unlikely(stack->curr == stack->end))
    return klstack_expand(stack, klmm);
  return true;
}

#endif
