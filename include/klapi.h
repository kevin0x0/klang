#ifndef KEVCC_KLANG_INCLUDE_VM_KLAPI_H
#define KEVCC_KLANG_INCLUDE_VM_KLAPI_H

#include "include/value/klcfunc.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include "include/value/klstate.h"
#include "include/value/klarray.h"
#include "include/misc/klutils.h"


#define klapi_pushobj(state, obj, type)         klapi_pushgcobj((state), (KlGCObject*)(obj), (type))
#define klapi_setobj(state, index, obj, type)   klapi_setgcobj((state), (index), (KlGCObject*)(obj), (type))


KlState* klapi_new_state(KlMM* klmm);

static inline KlException klapi_call(KlState* state, KlValue* callable, uint16_t narg, uint16_t nret);

static inline bool klapi_checkrange(KlState* state, int index);

/* get and set method on stack */
static inline KlValue* klapi_access(KlState* state, int index);
static inline KlType klapi_gettype(KlState* state, int index);
static inline void klapi_pop(KlState* state, size_t count);
static inline KlException klapi_pushcfunc(KlState* state, KlCFunction* cfunc);
static inline KlException klapi_pushint(KlState* state, KlInt val);
static inline KlException klapi_pushfloat(KlState* state, KlFloat val);
static inline KlException klapi_pushnil(KlState* state, size_t count);
static inline KlException klapi_pushbool(KlState* state, KlBool val);
static inline KlException klapi_pushstring(KlState* state, const char* str);
static inline KlException klapi_pushmap(KlState* state, size_t capacity);
static inline KlException klapi_pusharray(KlState* state, size_t capacity);
static inline KlException klapi_pushgcobj(KlState* state, KlGCObject* gcobj, KlType type);
//static inline KlException klapi_pushvalue(KlState* state, KlValue* val);

static inline KlException klapi_setcfunc(KlState* state, int index, KlCFunction* cfunc);
static inline KlException klapi_setint(KlState* state, int index, KlInt val);
static inline KlException klapi_setnil(KlState* state, int index);
static inline KlException klapi_setbool(KlState* state, int index, KlBool val);
static inline KlException klapi_setstring(KlState* state, int index, const char* str);
static inline KlException klapi_setgcobj(KlState* state, int index, KlGCObject* gcobj, KlType type);
static inline KlException klapi_setvalue(KlState* state, int index, KlValue* val);

static inline KlInt klapi_getint(KlState* state, int index);
static inline KlFloat klapi_getfloat(KlState* state, int index);
static inline KlBool klapi_getbool(KlState* state, int index);
static inline KlString* klapi_getstring(KlState* state, int index);
static inline KlMap* klapi_getmap(KlState* state, int index);
static inline KlArray* klapi_getarray(KlState* state, int index);


KlException klapi_loadref(KlState* state, KlClosure* clo, int stkidx, size_t refidx);
KlException klapi_storeref(KlState* state, KlClosure* clo, size_t refidx, int stkidx);
KlException klapi_loadglobal(KlState* state);
KlException klapi_storeglobal(KlState* state, KlString* varname);



static inline bool klapi_checktype(KlState* state, int index, KlType type);

/* if the type of func is KL_CFUNCTION, the the argument ref_no will be used.
 * else the ref_no will be ignored */
KlException klapi_make_closure(KlState* state, int result, int func, size_t ref_no);

static inline KlException klapi_call(KlState* state, KlValue* callable, uint16_t narg, uint16_t nret) {
  return klexec_call(state, callable, narg, nret);
}


static inline bool klapi_checkrange(KlState* state, int index) {
  return klstack_check_index(&state->stack, index);
}

static inline KlValue* klapi_access(KlState* state, int index) {
  if (klapi_checkrange(state, index))
    return klstate_getval(state, index);
  return NULL;
}

static inline KlType klapi_gettype(KlState* state, int index) {
  return klvalue_gettype(klapi_access(state, index));
}

static inline void klapi_pop(KlState* state, size_t count) {
  size_t stksize = klstack_size(&state->stack);
  count = stksize < count ? stksize : count;
  KlValue* bound = klstate_stktop(state) - count;
  klreflist_close(&state->reflist, bound, klstate_getmm(state));
  klstack_set_top(&state->stack, bound);
}

static inline KlException klapi_pushcfunc(KlState* state, KlCFunction* cfunc) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  klstack_pushcfunc(&state->stack, cfunc);
  return KL_E_NONE;
}

static inline KlException klapi_pushint(KlState* state, KlInt val) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  klstack_pushint(&state->stack, val);
  return KL_E_NONE;
}

static inline KlException klapi_pushfloat(KlState* state, KlFloat val) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  klstack_pushfloat(&state->stack, val);
  return KL_E_NONE;
}

static inline KlException klapi_pushnil(KlState* state, size_t count) {
  if (kl_unlikely(klstate_checkframe(state, count)))
    return KL_E_OOM;
  klstack_pushnil(&state->stack, count);
  return KL_E_NONE;
}

static inline KlException klapi_pushbool(KlState* state, KlBool val) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  klstack_pushbool(&state->stack, val);
  return KL_E_NONE;
}

static inline KlException klapi_pushstring(KlState* state, const char* str) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  KlString* klstr = klstrpool_new_string(state->strpool, str);
  if (!klstr) return KL_E_OOM;
  klstack_pushgcobj(&state->stack, (KlGCObject*)klstr, KL_STRING);
  return KL_E_NONE;
}

static inline KlException klapi_pushmap(KlState* state, size_t capacity) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  KlMap* map = klmap_create(state->common->klclass.map, capacity, state->mapnodepool);
  if (!map) return KL_E_OOM;
  klstack_pushgcobj(&state->stack, (KlGCObject*)map, KL_MAP);
  return KL_E_NONE;
}

static inline KlException klapi_pusharray(KlState* state, size_t capacity) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  KlArray* array = klarray_create(state->common->klclass.array, klstate_getmm(state), capacity);
  if (!array) return KL_E_OOM;
  klstack_pushgcobj(&state->stack, (KlGCObject*)array, KL_ARRAY);
  return KL_E_NONE;
}

static inline KlException klapi_pushgcobj(KlState* state, KlGCObject* gcobj, KlType type) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return KL_E_OOM;
  klstack_pushgcobj(&state->stack, gcobj, type);
  return KL_E_NONE;
}

static inline KlException klapi_setcfunc(KlState* state, int index, KlCFunction* cfunc) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setcfunc(val, cfunc);
  return KL_E_NONE;
}

static inline KlException klapi_setint(KlState* state, int index, KlInt intval) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setint(val, intval);
  return KL_E_NONE;
}

static inline KlException klapi_setnil(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setnil(val);
  return KL_E_NONE;
}

static inline KlException klapi_setbool(KlState* state, int index, KlBool boolval) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setbool(val, boolval);
  return KL_E_NONE;
}

static inline KlException klapi_setstring(KlState* state, int index, const char* str) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  KlString* klstr = klstrpool_new_string(state->strpool, str);
  if (!klstr) return KL_E_OOM;
  klvalue_setobj(val, klstr, KL_STRING);
  return KL_E_NONE;
}

static inline KlException klapi_setgcobj(KlState* state, int index, KlGCObject* gcobj, KlType type) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setgcobj(val, gcobj, type);
  return KL_E_NONE;
}

static inline KlException klapi_setvalue(KlState* state, int index, KlValue* other) {
  KlValue* val = klapi_access(state, index);
  if (!val) return KL_E_RANGE;
  klvalue_setvalue(val, other);
  return KL_E_NONE;
}

static inline KlInt klapi_getint(KlState* state, int index) {
  return klvalue_getint(klapi_access(state, index));
}

static inline KlFloat klapi_getfloat(KlState* state, int index) {
  return klvalue_getfloat(klapi_access(state, index));
}

static inline KlBool klapi_getbool(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);

  kl_assert(val != NULL, "index out of range");
  kl_assert(klvalue_checktype(val, KL_BOOL), "expected type KL_BOOL");

  return klvalue_getbool(val);
}

static inline KlString* klapi_getstring(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);

  kl_assert(val != NULL, "index out of range");
  kl_assert(klvalue_checktype(val, KL_STRING), "expected type KL_STRING");

  return klvalue_getobj(val, KlString*);
}

static inline KlMap* klapi_getmap(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);

  kl_assert(val != NULL, "index out of range");
  kl_assert(klvalue_checktype(val, KL_MAP), "expected type KL_MAP");

  return klvalue_getobj(val, KlMap*);
}

static inline KlArray* klapi_getarray(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);

  kl_assert(val != NULL, "index out of range");
  kl_assert(klvalue_checktype(val, KL_ARRAY), "expected type KL_ARRAY");

  return klvalue_getobj(val, KlArray*);
}

static inline bool klapi_checktype(KlState* state, int index, KlType type) {
  return klapi_gettype(state, index) == type;
}

#endif
