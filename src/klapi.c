#include "include/klapi.h"
#include "include/lang/klconfig.h"
#include "include/lang/klconvert.h"
#include "include/misc/klutils.h"
#include "include/value/klclass.h"
#include "include/value/klkfunc.h"
#include "include/value/klstate.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#ifdef KLCONFIG_USE_STATIC_LANGLIB
#include <string.h>
extern KlException KLCONFIG_LIBRARY_RTCPL_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_RTCPL_WRAPPER_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_PRINT_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_BASIC_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_STREAM_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_STRING_ENTRYFUNCNAME(KlState*);
extern KlException KLCONFIG_LIBRARY_TRACEBACK_ENTRYFUNCNAME(KlState*);
#else
#include "deps/k/include/lib/lib.h"
#endif
#include <stddef.h>


/*=============================ACCESS INTERFACE==============================*/

KlException klapi_checkstack(KlState* state, size_t size) {
  return klstate_checkframe(state, size);
}

KlException klapi_allocstack(KlState* state, size_t size) {
  KlException exception = klapi_checkstack(state, size);
  if (kl_unlikely(exception)) return exception;
  klstack_set_top(klstate_stack(state), klstate_stktop(state) + size);
  return KL_E_NONE;
}

KlException klapi_checkframe(KlState* state, size_t size) {
  size_t currframesize = klapi_framesize(state);
  if (currframesize >= size) return KL_E_NONE;
  return klapi_checkstack(state, size - currframesize);
}

KlException klapi_checkframeandset(KlState* state, size_t size) {
  size_t currframesize = klapi_framesize(state);
  if (currframesize >= size) {
    klstack_set_top(klstate_stack(state), klapi_accessb(state, size));
    return KL_E_NONE;
  }
  return klapi_allocstack(state, size - currframesize);
}

bool klapi_checkrange(KlState* state, int index) {
  return klstack_check_index(klstate_stack(state), index);
}

KlValue* klapi_stacktop(KlState* state) {
  return klstate_stktop(state);
}

KlValue* klapi_access(KlState* state, int index) {
  kl_assert(klapi_checkrange(state, index), "index out of range");
  return klstate_getval(state, index);
}

KlValue* klapi_pointer(KlState* state, int index) {
  kl_assert(klstack_size(klstate_stack(state)) + index <= klstack_capacity(klstate_stack(state)), "index out of range");
  return klstate_getval(state, index);
}

KlValue* klapi_accessb(KlState* state, unsigned index) {
  return klstate_currci(state)->base + index;
}

KlType klapi_gettype(KlState* state, int index) {
  return klvalue_gettype(klapi_access(state, index));
}

KlType klapi_gettypeb(KlState* state, unsigned int index) {
  return klvalue_gettype(klapi_accessb(state, index));
}

void klapi_setframesize(KlState* state, unsigned size) {
  kl_assert((size_t)(klstate_currci(state)->base - klstack_raw(klstate_stack(state))) + size <= klstack_size(klstate_stack(state)), "check stack capacity before setting framesize");
  klstack_set_top(klstate_stack(state), klstate_currci(state)->base + size);
}

void klapi_pop(KlState* state, size_t count) {
  kl_assert(count <= klstack_size(klstate_stack(state)), "pop too many values");
  klstack_move_top(klstate_stack(state), -count);
}

void klapi_close(KlState* state, KlValue* bound) {
  kl_assert(klstack_onstack(klstate_stack(state), bound), "pop too many values");
  klreflist_close(klstate_reflist(state), bound, klstate_getmm(state));
}

void klapi_popclose(KlState* state, size_t count) {
  kl_assert(count <= klstack_size(klstate_stack(state)), "pop too many values");
  KlValue* bound = klstate_stktop(state) - count;
  klreflist_close(klstate_reflist(state), bound, klstate_getmm(state));
  klstack_set_top(klstate_stack(state), bound);
}

void klapi_move(KlState* state, int from, int to, size_t count) {
  KlValue* pfrom = klapi_pointer(state, from);
  KlValue* pto = klapi_pointer(state, to);
  memmove(pfrom, pto, count * sizeof (KlValue));
}

void klapi_pushcfunc(KlState* state, KlCFunction* cfunc) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushcfunc(klstate_stack(state), cfunc);
}

void klapi_pushint(KlState* state, KlInt val) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushint(klstate_stack(state), val);
}

void klapi_pushfloat(KlState* state, KlFloat val) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushfloat(klstate_stack(state), val);
}

void klapi_pushnil(KlState* state, size_t count) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushnil(klstate_stack(state), count);
}

void klapi_pushbool(KlState* state, KlBool val) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushbool(klstate_stack(state), val);
}

KlException klapi_pushstring(KlState* state, const char* str) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  KlString* klstr = klstrpool_new_string(state->strpool, str);
  if (!klstr) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)klstr, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_pushstring_buf(KlState* state, const char* buf, size_t buflen) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  KlString* klstr = klstrpool_new_string_buf(state->strpool, buf, buflen);
  if (!klstr) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)klstr, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_pushmap(KlState* state, size_t capacity) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  KlMap* map = klmap_create(klstate_getmm(state), capacity);
  if (!map) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)map, KL_MAP);
  return KL_E_NONE;
}

KlException klapi_pusharray(KlState* state, size_t capacity) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  KlArray* array = klarray_create(klstate_getmm(state), capacity);
  if (!array) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)array, KL_ARRAY);
  return KL_E_NONE;
}

void klapi_pushuserdata(KlState* state, void* ud) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushuserdata(klstate_stack(state), ud);
}

void klapi_pushgcobj(KlState* state, KlGCObject* gcobj, KlType type) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushgcobj(&state->stack, gcobj, type);
}

void klapi_pushvalue(KlState* state, const KlValue* val) {
  kl_assert(klstack_residual(klstate_stack(state)) != 0, "stack index out of range");
  klstack_pushvalue(&state->stack, val);
}

void klapi_setcfunc(KlState* state, int index, KlCFunction* cfunc) {
  KlValue* val = klapi_access(state, index);
  klvalue_setcfunc(val, cfunc);
}

void klapi_setint(KlState* state, int index, KlInt intval) {
  KlValue* val = klapi_access(state, index);
  klvalue_setint(val, intval);
}

void klapi_setnil(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);
  klvalue_setnil(val);
}

void klapi_setbool(KlState* state, int index, KlBool boolval) {
  KlValue* val = klapi_access(state, index);
  klvalue_setbool(val, boolval);
}

KlException klapi_setstring(KlState* state, int index, const char* str) {
  KlValue* val = klapi_access(state, index);
  KlString* klstr = klstrpool_new_string(state->strpool, str);
  if (!klstr) return klstate_throw(state, KL_E_OOM, "out of momery");
  klvalue_setobj(val, klstr, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_setmap(KlState* state, int index, size_t capacity) {
  KlValue* val = klapi_access(state, index);
  KlMap* map = klmap_create(klstate_getmm(state), capacity);
  if (!map) return klstate_throw(state, KL_E_OOM, "out of momery");
  klvalue_setobj(val, map, KL_MAP);
  return KL_E_NONE;
}

KlException klapi_setarray(KlState* state, int index, size_t capacity) {
  KlValue* val = klapi_access(state, index);
  KlArray* array = klarray_create(klstate_getmm(state), capacity);
  if (!array) return klstate_throw(state, KL_E_OOM, "out of momery");
  klvalue_setobj(val, array, KL_ARRAY);
  return KL_E_NONE;
}

void klapi_setuserdata(KlState* state, int index, void* ud) {
  KlValue* val = klapi_access(state, index);
  klvalue_setuserdata(val, ud);
}

void klapi_setgcobj(KlState* state, int index, KlGCObject* gcobj, KlType type) {
  KlValue* val = klapi_access(state, index);
  klvalue_setgcobj(val, gcobj, type);
}

void klapi_setvalue(KlState* state, int index, const KlValue* other) {
  KlValue* val = klapi_access(state, index);
  klvalue_setvalue(val, other);
}

KlCFunction* klapi_getcfunc(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_CFUNCTION), "expected type KL_CFUNCTION");
  return klvalue_getcfunc(klapi_access(state, index));
}

KlCFunction* klapi_getcfuncb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_CFUNCTION), "expected type KL_CFUNCTION");
  return klvalue_getcfunc(klapi_accessb(state, index));
}

KlInt klapi_getint(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_INT), "expected type KL_INT");
  return klvalue_getint(klapi_access(state, index));
}

KlInt klapi_getintb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_INT), "expected type KL_INT");
  return klvalue_getint(klapi_accessb(state, index));
}

KlFloat klapi_getfloat(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_FLOAT), "expected type KL_FLOAT");
  return klvalue_getfloat(klapi_access(state, index));
}

KlFloat klapi_getfloatb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_FLOAT), "expected type KL_FLOAT");
  return klvalue_getfloat(klapi_accessb(state, index));
}

KlBool klapi_getbool(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_BOOL), "expected type KL_BOOL");
  return klvalue_getbool(klapi_access(state, index));
}

KlBool klapi_getboolb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_BOOL), "expected type KL_BOOL");
  return klvalue_getbool(klapi_accessb(state, index));
}

KlString* klapi_getstring(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_STRING), "expected type KL_STRING");
  return klvalue_getobj(klapi_access(state, index), KlString*);
}

KlString* klapi_getstringb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_STRING), "expected type KL_STRING");
  return klvalue_getobj(klapi_accessb(state, index), KlString*);
}

KlMap* klapi_getmap(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_MAP), "expected type KL_MAP");
  return klvalue_getobj(klapi_access(state, index), KlMap*);
}

KlMap* klapi_getmapb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_MAP), "expected type KL_MAP");
  return klvalue_getobj(klapi_accessb(state, index), KlMap*);
}

KlArray* klapi_getarray(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_ARRAY), "expected type KL_ARRAY");
  return klvalue_getobj(klapi_access(state, index), KlArray*);
}

KlArray* klapi_getarrayb(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_ARRAY), "expected type KL_ARRAY");
  return klvalue_getobj(klapi_accessb(state, index), KlArray*);
}

KlGCObject* klapi_getgcobj(KlState* state, int index) {
  return klvalue_getgcobj(klapi_access(state, index));
}

KlGCObject* klapi_getgcobjb(KlState* state, unsigned index) {
  return klvalue_getgcobj(klapi_accessb(state, index));
}

void* klapi_getuserdata(KlState* state, int index) {
  kl_assert(klvalue_checktype(klapi_access(state, index), KL_USERDATA), "expected type KL_USERDATA");
  return klvalue_getuserdata(klapi_access(state, index)); 
}

void* klapi_getuserdatab(KlState* state, unsigned index) {
  kl_assert(klvalue_checktype(klapi_accessb(state, index), KL_USERDATA), "expected type KL_USERDATA");
  return klvalue_getuserdata(klapi_accessb(state, index)); 
}

KlBool klapi_tobool(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);
  return klexec_satisfy(val, KL_TRUE);
}

KlException klapi_mkcclosure(KlState* state, int index, KlCFunction* cfunc, size_t nref) {
  KlCClosure* cclo = klcclosure_create(klstate_getmm(state), cfunc, klapi_pointer(state, -nref), klstate_reflist(state), nref);
  if (kl_unlikely(!cclo)) return klstate_throw(state, KL_E_OOM, "out of memory while creating C closure");
  klapi_setobj(state, index, cclo, KL_CCLOSURE);
  return KL_E_NONE;
}

KlValue* klapi_getref(KlState* state, unsigned short refidx) {
  kl_assert(klstate_currci(state)->status & KLSTATE_CI_STATUS_CCLO, "not a C closure call");
  KlCClosure* cclo = klcast(KlCClosure*, klstate_currci(state)->callable.clo);
  kl_assert(refidx < cclo->nref, "'refidx' exceeds the number of references hold by this C closure");
  return klref_getval(klcclosure_getref(cclo, refidx));
}

void klapi_loadglobal(KlState* state) {
  KlString* varname = klapi_getstring(state, -1);
  KlMapSlot* slot = klmap_searchstring(state->global, varname);
  slot ? klapi_setvalue(state, -1, &slot->value) : klapi_setnil(state, -1);
}

KlException klapi_storeglobal(KlState* state, KlString* varname, int validx) {
  KlMapSlot* slot = klmap_searchstring(state->global, varname);
  if (!slot) {
    if (kl_unlikely(!klmap_insertstring(state->global, klstate_getmm(state), varname, klapi_access(state, validx))))
      return KL_E_OOM;
  } else {
    klvalue_setvalue(&slot->value, klapi_access(state, validx));
  }
  return KL_E_NONE;
}

KlException klapi_toint(KlState* state, int to, int from) {
  KlValue* val = klapi_access(state, from);
  if (klvalue_checktype(val, KL_INT)) {
    klvalue_setint(klapi_access(state, to), klvalue_getint(val));
    return KL_E_NONE;
  } else if (klvalue_checktype(val, KL_FLOAT)) {
    klvalue_setint(klapi_access(state, to), klcast(KlInt, klvalue_getfloat(val)));
    return KL_E_NONE;
  } else if (klvalue_checktype(val, KL_STRING)) {
    char* p = NULL;
    KlString* str = klvalue_getobj(val, KlString*);
    const char* content = klstring_content(str);
    KlInt res = kllang_str2int(content, &p, 0);
    if (p != content + klstring_length(str))
      return klstate_throw(state, KL_E_INVLD, "this string can not be converted to integer");
    klvalue_setint(klapi_access(state, to), res);
    return KL_E_NONE;
  } else {
    return klstate_throw(state, KL_E_INVLD, "can not be converted to integer");
  }
}

KlException klapi_tofloat(KlState* state, int to, int from) {
  KlValue* val = klapi_access(state, from);
  if (klvalue_checktype(val, KL_FLOAT)) {
    klvalue_setfloat(klapi_access(state, to), klvalue_getfloat(val));
    return KL_E_NONE;
  } else if (klvalue_checktype(val, KL_INT)) {
    klvalue_setfloat(klapi_access(state, to), klcast(KlFloat, klvalue_getint(val)));
    return KL_E_NONE;
  } else if (klvalue_checktype(val, KL_STRING)) {
    char* p = NULL;
    KlString* str = klvalue_getobj(val, KlString*);
    const char* content = klstring_content(str);
    KlInt res = kllang_str2float(content, &p);
    if (p != content + klstring_length(str))
      return klstate_throw(state, KL_E_INVLD, "this string can not be converted to float");
    klvalue_setfloat(klapi_access(state, to), res);
    return KL_E_NONE;
  } else {
    return klstate_throw(state, KL_E_INVLD, "can not be converted to float");
  }
}

KlString* klapi_tostring(KlState* state, int index) {
#define KLAPI_BUFSIZE   (100)
  KlValue* val = klapi_access(state, index);
  if (klvalue_checktype(val, KL_STRING)) {
    return klvalue_getobj(val, KlString*);
  } else if (klvalue_checktype(val, KL_INT)) {
    char buf[KLAPI_BUFSIZE];
    kllang_int2str(buf, KLAPI_BUFSIZE, klvalue_getint(val));
    KlString* str = klstrpool_new_string(klstate_strpool(state), buf);
    if (kl_unlikely(!str)) {
      klstate_throw(state, KL_E_OOM, "out of memory");
      return NULL;
    }
    return str;
  } else if (klvalue_checktype(val, KL_FLOAT)) {
    char buf[KLAPI_BUFSIZE];
    kllang_float2str(buf, KLAPI_BUFSIZE, klvalue_getfloat(val));
    KlString* str = klstrpool_new_string(klstate_strpool(state), buf);
    if (kl_unlikely(!str)) {
      klstate_throw(state, KL_E_OOM, "out of memory");
      return NULL;
    }
    return str;
  } else {
    klstate_throw(state, KL_E_INVLD, "can not be converted to string");
    return NULL;
  }
#undef KLAPI_BUFSIZE
}



/*============================C FUNCTION UTILITIES===========================*/


KlException klapi_tailcall(KlState* state, KlValue* callable, size_t narg) {
  return klexec_tailcall(state, callable, narg);
}

KlException klapi_call(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  return klexec_call(state, callable, narg, nret, respos);
}

KlException klapi_scall(KlState* state, KlValue* callable, size_t narg, size_t nret) {
  ptrdiff_t retpos_save = klexec_savestack(state, klstate_stktop(state) - narg);
  KLAPI_PROTECT(klexec_call(state, callable, narg, nret, klexec_restorestack(state, retpos_save)));
  if (nret != KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), klexec_restorestack(state, retpos_save) + nret);
  return KL_E_NONE;
}

KlException klapi_scall_yieldable(KlState* state, KlValue* callable, size_t narg, size_t nret, KlCFunction* afteryield, KlCIUD ud) {
  ptrdiff_t retpos_save = klexec_savestack(state, klstate_stktop(state) - narg);
  KLAPI_PROTECT(klexec_call_yieldable(state, callable, narg, nret, klexec_restorestack(state, retpos_save), afteryield, ud));
  if (nret != KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), klexec_restorestack(state, retpos_save) + nret);
  return KL_E_NONE;
}

KlException klapi_tryscall(KlState* state, int errhandler, KlValue* callable, size_t narg, size_t nret) {
  ptrdiff_t errhandler_save = klstack_size(klstate_stack(state)) + errhandler;
  ptrdiff_t retpos_save = klexec_savestack(state, klstate_stktop(state) - narg);
  kl_assert(errhandler_save >= 0, "errhandler not provided");
  KlCallInfo* currci = klstate_currci(state);
  KlException exception = klapi_call(state, callable, narg, nret, klexec_restorestack(state, retpos_save));
  /* if no exception occurred, just return */
  if (kl_likely(!exception)) {
    if (nret != KLAPI_VARIABLE_RESULTS)
      klstack_set_top(klstate_stack(state), klexec_restorestack(state, retpos_save) + nret);
    return KL_E_NONE;
  }
  /* else handle exception */
  /* close potential references */
  klapi_close(state, klexec_restorestack(state, retpos_save));
  /* call error handler */
  exception = klapi_call(state, klexec_restorestack(state, errhandler_save), 0, nret, klexec_restorestack(state, retpos_save));
  if (kl_unlikely(exception)) return exception;
  if (nret != KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), klexec_restorestack(state, retpos_save) + nret);
  klapi_exception_clear(state, currci);
  return KL_E_NONE;
}

KlException klapi_trycall(KlState* state, int errhandler, KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  ptrdiff_t errhandler_save = klstack_size(klstate_stack(state)) + errhandler;
  ptrdiff_t respos_save = klexec_savestack(state, respos);
  ptrdiff_t base_save = klexec_savestack(state, klstate_stktop(state) - narg);
  kl_assert(errhandler_save >= 0, "errhandler not provided");
  KlCallInfo* currci = klstate_currci(state);
  KlException exception = klapi_call(state, callable, narg, nret, respos);
  /* if no exception occurred, just return */
  if (kl_likely(!exception)) return exception;
  /* else handle exception */
  /* close potential references */
  klapi_close(state, klexec_restorestack(state, base_save));
  /* call error handler */
  exception = klapi_call(state, klexec_restorestack(state, errhandler_save), 0, nret, klexec_restorestack(state, respos_save));
  if (kl_unlikely(exception)) return exception;
  klapi_exception_clear(state, currci);
  return KL_E_NONE;
}

KlException klapi_return(KlState* state, size_t nret) {
  kl_assert(klapi_framesize(state) >= nret, "number of returned values exceeds the stack frame size, which is impossible.");
  kl_assert(nret < KLAPI_VARIABLE_RESULTS, "currently does not support this number of returned values.");
  KlCallInfo* currci = klstate_currci(state);
  KlValue* retpos = currci->base + currci->retoff;
  KlValue* respos = klapi_pointer(state, -nret);
  kl_assert(retpos <= respos, "number of returned values exceeds stack frame size, which is impossible");
  if (retpos == respos) {
    if (currci->nret != KLAPI_VARIABLE_RESULTS && nret < currci->nret)
      klexec_setnils(respos, currci->nret - nret);
    return KL_E_NONE;
  }
  size_t nwanted = currci->nret == KLAPI_VARIABLE_RESULTS ? nret : currci->nret;
  size_t ncopy = nwanted < nret ? nwanted : nret;
  while (ncopy--)
    klvalue_setvalue(retpos++, respos++);
  if (nret < nwanted)
    klexec_setnils(retpos, nwanted - nret);
  if (currci->nret == KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), retpos + nwanted - nret);
  return KL_E_NONE;
}


/*============================OPERATION ON VALUE=============================*/


KlKFunction* klapi_kfunc_alloc(KlState* state, unsigned codelen, unsigned short nconst,
                              unsigned short nref, unsigned short nsubfunc, KlUByte framesize, KlUByte nparam) {
  KlKFunction* kfunc = klkfunc_alloc(klstate_getmm(state), codelen, nconst, nref, nsubfunc, framesize, nparam);
  if (kl_unlikely(!kfunc))
    klapi_throw_internal(state, KL_E_OOM, "out of memory while creating a to be initialized K function");
  return kfunc;
}


/*=================================OPERATOR==================================*/


KlException klapi_concat(KlState* state) {
  kl_assert(klapi_framesize(state) >= 2, "must provide two values");
  KlString* str = klstrpool_string_concat(klstate_strpool(state), klapi_getstring(state, -2), klapi_getstring(state, -1));
  if (kl_unlikely(!str))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while doing concat");
  klapi_pop(state, 1);
  klapi_setobj(state, -1, str, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_concati(KlState* state, int result, int left, int right) {
  KlString* str = klstrpool_string_concat(klstate_strpool(state), klapi_getstring(state, left), klapi_getstring(state, right));
  if (kl_unlikely(!str))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while doing concat");
  klapi_setobj(state, result, str, KL_STRING);
  return KL_E_NONE;
}

/*============================LIBRARY LOADER=================================*/


KlException klapi_loadlib(KlState* state, int result, const char* entryfunction) {
#ifdef KLCONFIG_USE_STATIC_LANGLIB
#def KLAPI_LIBINFO_INIT(libid)  \
    { .libname = KLCONFIG_LIBRARY_##libid##_LIBNAME_QUOTE, .entry = KLCONFIG_LIBRARY_##libid##_ENTRYFUNCNAME },

  static const struct {
    const char* libname;
    KlCFunction* entry;
  } libinfos[] = {
    klconfig_library_foreach(KLAPI_LIBINFO_INIT)
  };
#undef KLAPI_LIBINFO_INIT

  kl_unused(entryfunction);
  KlString* libpath = klapi_getstring(state, -1);
  size_t libpathlen = klstring_length(libpath);
  const char* libpathcontent = klstring_content(libpath);
  KlCFunction* init = NULL;
  for (size_t i = 0; i < sizeof (libinfos) / sizeof (libinfos[0]); ++i) {
    size_t staticliblen;
    if (libpathlen >= (staticliblen = strlen(libinfos[i].libname)) &&
        strcmp(libpathcontent + libpathlen - staticliblen, libinfos[i].libname) == 0) {
      init = libinfos[i].entry;
    }
  }
  if (kl_unlikely(!init))
    return klstate_throw(state, KL_E_INVLD, "can not open library: %s", klstring_content(libpath));
#else
  /* open library */
  KlString* libpath = klapi_getstring(state, -1);
  KLib handle = klib_dlopen(klstring_content(libpath));
  if (kl_unlikely(klib_failed(handle))) 
    return klstate_throw(state, KL_E_INVLD, "can not open library: %s", klstring_content(libpath));

  /* find entry point */
  entryfunction = entryfunction ? entryfunction : "kllib_init";
  KlCFunction* init = (KlCFunction*)klib_dlsym(handle, entryfunction);
  if (kl_unlikely(!init)) {
    klib_dlclose(handle);
    return klstate_throw(state, KL_E_INVLD, "can not find entry point: %s in library: ", entryfunction, klstring_content(libpath));
  }
#endif

  /* call the entry function */
  return klapi_call(state, &klvalue_cfunc(init), 1, KLAPI_VARIABLE_RESULTS, klapi_stacktop(state) - 1 + result);
}




/*============================VIRTUAL MACHINE INIT===========================*/

KlState* klapi_new_state(KlMM* klmm) {
  klmm_stopgc(klmm);  /* disable gc */

  KlStrPool* strpool = klstrpool_create(klmm, 32);
  if (!strpool) {
    klmm_restartgc(klmm);
    return NULL;
  }

  KlCommon* common = klcommon_create(klmm, strpool);
  if (!common) {
    klmm_restartgc(klmm);
    return NULL;
  }
  klcommon_pin(common);

  KlMap* global = klmap_create(klmm, 5);
  if (!global) {
    klmm_restartgc(klmm);
    klcommon_unpin(common, klmm);
    return NULL;
  }

  KlState* state = klstate_create(klmm, global, common, strpool, NULL);
  if (!state) {
    klmm_restartgc(klmm);
    klcommon_unpin(common, klmm);
    return NULL;
  }
  klmm_register_root(klmm, klmm_to_gcobj(state));
  klmm_restartgc(klmm);
  klcommon_unpin(common, klmm);
  return state;
}

/*=============================VALUE OPERATION===============================*/


KlException klapi_class_newshared_normal(KlState* state, KlClass* klclass, KlString* fieldname) {
  kl_assert(klapi_framesize(state) >= 1, "you must push a value on top of stack");
  KlException exception = klclass_newshared_normal(klclass, klstate_getmm(state), fieldname, klapi_access(state, -1));
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when setting a new shared field: %s", klstring_content(fieldname));
  } else if (exception == KL_E_INVLD) {
    return klstate_throw(state, exception, "can not overwrite local field: %s", klstring_content(fieldname)); 
  }
  return KL_E_NONE;
}

KlException klapi_class_newshared_method(KlState* state, KlClass* klclass, KlString* fieldname) {
  kl_assert(klapi_framesize(state) >= 1, "you must push a value on top of stack");
  KlException exception = klclass_newshared_method(klclass, klstate_getmm(state), fieldname, klapi_access(state, -1));
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when setting a new method field: %s", klstring_content(fieldname));
  } else if (exception == KL_E_INVLD) {
    return klstate_throw(state, exception, "can not overwrite local field: %s", klstring_content(fieldname)); 
  }
  return KL_E_NONE;
}

KlException klapi_class_newlocal(KlState* state, KlClass* klclass, KlString* fieldname) {
  KlException exception = klclass_newlocal(klclass, klstate_getmm(state), fieldname);
  if (exception == KL_E_OOM) {
    return klstate_throw(state, exception, "out of memory when adding a new local field: %s", klstring_content(fieldname));
  } else if (exception == KL_E_INVLD) {
    return klstate_throw(state, exception, "can not overwrite local field: %s", klstring_content(fieldname)); 
  }
  return KL_E_NONE;
}

KlException klapi_class_newobject(KlState* state, KlClass* klclass) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushnil(state, 1);
  KlException exception = klclass_new_object(klclass, klstate_getmm(state), klapi_access(state, -1));
  if (kl_unlikely(exception))
    return klexec_handle_newobject_exception(state, exception);
  return exception;
}

void klapi_class_getfield(KlState* state, KlClass* klclass, KlString* fieldname, KlValue* result) {
  kl_unused(state);
  KlClassSlot* slot = klclass_find(klclass, fieldname);
  kl_likely(slot) ? klvalue_setvalue(result, &slot->value) : klvalue_setnil(result);
}

bool klapi_getmethod(KlState* state, KlValue* dotable, KlString* fieldname, KlValue* result) {
  return klexec_getmethod(state, dotable, fieldname, result);
}
