#include "include/klapi.h"
#include "include/lang/klconvert.h"
#include "include/lib/lib.h"


/*=============================ACCESS INTERFACE==============================*/

KlException klapi_checkstack(KlState* state, size_t size) {
  if (kl_unlikely(klstate_checkframe(state, size)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  return KL_E_NONE;
}

KlException klapi_allocstack(KlState* state, size_t size) {
  KlException exception = klapi_checkstack(state, size);
  if (kl_unlikely(exception)) return exception;
  klstack_set_top(klstate_stack(state), klstate_stktop(state) + size);
  return KL_E_NONE;
}

bool klapi_checkrange(KlState* state, int index) {
  return klstack_check_index(klstate_stack(state), index);
}

KlValue* klapi_access(KlState* state, int index) {
  kl_assert(klapi_checkrange(state, index), "index out of range");
  return klstate_getval(state, index);
}

KlValue* klapi_accessb(KlState* state, unsigned index) {
  return klstate_currci(state)->base + index;
}

KlType klapi_gettype(KlState* state, int index) {
  return klvalue_gettype(klapi_access(state, index));
}

KlType klapi_gettypeb(KlState* state, int index) {
  return klvalue_gettype(klapi_accessb(state, index));
}

void klapi_pop(KlState* state, size_t count) {
  kl_assert(count <= klstack_size(klstate_stack(state)), "pop too many values");
  KlValue* bound = klstate_stktop(state) - count;
  klreflist_close(&state->reflist, bound, klstate_getmm(state));
  klstack_set_top(klstate_stack(state), bound);
}

KlException klapi_pushcfunc(KlState* state, KlCFunction* cfunc) {
  if (!klstack_expand_if_full(&state->stack, klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushcfunc(klstate_stack(state), cfunc);
  return KL_E_NONE;
}

KlException klapi_pushint(KlState* state, KlInt val) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushint(klstate_stack(state), val);
  return KL_E_NONE;
}

KlException klapi_pushfloat(KlState* state, KlFloat val) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushfloat(klstate_stack(state), val);
  return KL_E_NONE;
}

KlException klapi_pushnil(KlState* state, size_t count) {
  if (kl_unlikely(klstate_checkframe(state, count)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushnil(klstate_stack(state), count);
  return KL_E_NONE;
}

KlException klapi_pushbool(KlState* state, KlBool val) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushbool(klstate_stack(state), val);
  return KL_E_NONE;
}

KlException klapi_pushstring(KlState* state, const char* str) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  KlString* klstr = klstrpool_new_string(state->strpool, str);
  if (!klstr) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)klstr, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_pushstring_buf(KlState* state, const char* buf, size_t buflen) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  KlString* klstr = klstrpool_new_string_buf(state->strpool, buf, buflen);
  if (!klstr) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)klstr, KL_STRING);
  return KL_E_NONE;
}

KlException klapi_pushmap(KlState* state, size_t capacity) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  KlMap* map = klmap_create(state->common->klclass.map, capacity, state->mapnodepool);
  if (!map) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)map, KL_MAP);
  return KL_E_NONE;
}

KlException klapi_pusharray(KlState* state, size_t capacity) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  KlArray* array = klarray_create(state->common->klclass.array, klstate_getmm(state), capacity);
  if (!array) return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(klstate_stack(state), (KlGCObject*)array, KL_ARRAY);
  return KL_E_NONE;
}

KlException klapi_pushuserdata(KlState* state, void* ud) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushuserdata(klstate_stack(state), ud);
  return KL_E_NONE;
}

KlException klapi_pushgcobj(KlState* state, KlGCObject* gcobj, KlType type) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushgcobj(&state->stack, gcobj, type);
  return KL_E_NONE;
}

KlException klapi_pushvalue(KlState* state, KlValue* val) {
  if (!klstack_expand_if_full(klstate_stack(state), klstate_getmm(state)))
    return klstate_throw(state, KL_E_OOM, "out of momery");
  klstack_pushvalue(&state->stack, val);
  return KL_E_NONE;
}

KlException klapi_setcfunc(KlState* state, int index, KlCFunction* cfunc) {
  KlValue* val = klapi_access(state, index);
  klvalue_setcfunc(val, cfunc);
  return KL_E_NONE;
}

KlException klapi_setint(KlState* state, int index, KlInt intval) {
  KlValue* val = klapi_access(state, index);
  klvalue_setint(val, intval);
  return KL_E_NONE;
}

KlException klapi_setnil(KlState* state, int index) {
  KlValue* val = klapi_access(state, index);
  klvalue_setnil(val);
  return KL_E_NONE;
}

KlException klapi_setbool(KlState* state, int index, KlBool boolval) {
  KlValue* val = klapi_access(state, index);
  klvalue_setbool(val, boolval);
  return KL_E_NONE;
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
  KlMap* map = klmap_create(state->common->klclass.map, capacity, state->mapnodepool);
  if (!map) return klstate_throw(state, KL_E_OOM, "out of momery");
  klvalue_setobj(val, map, KL_MAP);
  return KL_E_NONE;
}

KlException klapi_setarray(KlState* state, int index, size_t capacity) {
  KlValue* val = klapi_access(state, index);
  KlArray* array = klarray_create(state->common->klclass.array, klstate_getmm(state), capacity);
  if (!array) return klstate_throw(state, KL_E_OOM, "out of momery");
  klvalue_setobj(val, array, KL_ARRAY);
  return KL_E_NONE;
}

KlException klapi_setuserdata(KlState* state, int index, void* ud) {
  KlValue* val = klapi_access(state, index);
  klvalue_setuserdata(val, ud);
  return KL_E_NONE;
}

KlException klapi_setgcobj(KlState* state, int index, KlGCObject* gcobj, KlType type) {
  KlValue* val = klapi_access(state, index);
  klvalue_setgcobj(val, gcobj, type);
  return KL_E_NONE;
}

KlException klapi_setvalue(KlState* state, int index, KlValue* other) {
  KlValue* val = klapi_access(state, index);
  klvalue_setvalue(val, other);
  return KL_E_NONE;
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
  KlCClosure* cclo = klcclosure_create(klstate_getmm(state), cfunc, klapi_access(state, -nref), klstate_reflist(state), nref);
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

bool klapi_checktype(KlState* state, int index, KlType type) {
  return klapi_gettype(state, index) == type;
}

bool klapi_checktypeb(KlState* state, int index, KlType type) {
  return klapi_gettypeb(state, index) == type;
}

KlException klapi_loadglobal(KlState* state) {
  KlString* varname = klapi_getstring(state, -1);
  KlMapIter itr = klmap_searchstring(state->global, varname);
  itr ? klapi_setvalue(state, -1, &itr->value) : klapi_setnil(state, -1);
  return KL_E_NONE;
}

KlException klapi_storeglobal(KlState* state, KlString* varname) {
  KlMapIter itr = klmap_searchstring(state->global, varname);
  if (!itr) {
    if (kl_unlikely(!klmap_insertstring(state->global, varname, klapi_access(state, -1))))
      return KL_E_OOM;
  } else {
    klvalue_setvalue(&itr->value, klapi_access(state, -1));
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
}



/*============================C FUNCTION UTILITIES===========================*/


KlException klapi_tailcall(KlState* state, KlValue* callable, size_t narg) {
  return klexec_tailcall(state, callable, narg);
}

KlException klapi_call(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  return klexec_call(state, callable, narg, nret, respos);
}

KlException klapi_trycall(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos) {
  ptrdiff_t errhandler_save = klexec_savestack(state, klstate_stktop(state) - narg - 1);
  ptrdiff_t respos_save = klexec_savestack(state, respos);
  kl_assert(errhandler_save >= 0, "errhandler not provided");
  KlCallInfo* currci = klstate_currci(state);
  KlException exception = klapi_call(state, callable, narg, nret, respos);
  /* if no exception occurred, just return */
  if (kl_likely(!exception)) return exception;
  /* else handle exception */
  exception = klapi_call(state, klexec_restorestack(state, errhandler_save), 0, nret, klexec_restorestack(state, respos_save));
  if (kl_unlikely(exception)) return exception;
  klapi_exception_clear(state, currci);
  return KL_E_NONE;
}

size_t klapi_narg(KlState* state) {
  return klstate_currci(state)->narg;
}

size_t klapi_nres(KlState* state) {
  return klstate_currci(state)->nret;
}

size_t klapi_framesize(KlState* state) {
  return klstate_stktop(state) - klstate_currci(state)->base;
}

KlException klapi_return(KlState* state, size_t nret) {
  kl_assert(klstack_size(klstate_stack(state)) >= nret, "number of returned values exceeds the stack size, which is impossible.");
  kl_assert(nret < KLAPI_VARIABLE_RESULTS, "currently does not support this number of returned values.");
  KlCallInfo* currci = klstate_currci(state);
  KlValue* retpos = currci->base + currci->retoff;
  KlValue* respos = klapi_access(state, -nret);
  kl_assert(retpos <= respos, "number of returned values exceeds stack frame size, which is impossible");
  if (retpos == respos) return KL_E_NONE;
  size_t nwanted = currci->nret == KLAPI_VARIABLE_RESULTS ? nret : currci->nret;
  size_t ncopy = nwanted < nret ? nwanted : nret;
  while (ncopy--)
    klvalue_setvalue(respos++, retpos++);
  if (nret < nwanted)
    klexec_setnils(respos, nwanted - nret);
  if (currci->nret == KLAPI_VARIABLE_RESULTS)
    klstack_set_top(klstate_stack(state), respos + nwanted);
  return KL_E_NONE;
}



/*============================LIBRARY LOADER=================================*/


KlException klapi_loadlib(KlState* state, const char* libpath, const char* entryfunction) {
  /* open library */
  KLib handle = klib_dlopen(libpath);
  if (kl_unlikely(klib_failed(handle))) 
    return klstate_throw(state, KL_E_INVLD, "can not open library: %s", libpath);

  /* find entry point */
  entryfunction = entryfunction ? entryfunction : "kllib_init";
  KlCFunction* init = (KlCFunction*)klib_dlsym(handle, entryfunction);
  if (kl_unlikely(!init)) {
    klib_dlclose(handle);
    return klstate_throw(state, KL_E_INVLD, "can not find entry point: %s", entryfunction);
  }

  /* call the entry function */
  KlException exception = klapi_pushcfunc(state, init);
  if (kl_unlikely(exception)) return exception;
  return klapi_call(state, klapi_access(state, -1), 0, KLAPI_VARIABLE_RESULTS, klapi_access(state, -1));
}




/*============================VIRTUAL MACHINE INIT===========================*/

KlState* klapi_new_state(KlMM* klmm) {
  klmm_stopgc(klmm);  /* disable gc */

  KlMapNodePool* mapnodepool = klmapnodepool_create(klmm);
  if (!mapnodepool) {
    klmm_restartgc(klmm);
    return NULL;
  }
  klmapnodepool_pin(mapnodepool);

  KlStrPool* strpool = klstrpool_create(klmm, 32);
  if (!strpool) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }

  KlCommon* common = klcommon_create(klmm, strpool, mapnodepool);
  if (!common) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    return NULL;
  }
  klcommon_pin(common);

  KlMap* global = klmap_create(common->klclass.map, 5, mapnodepool);
  if (!global) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }

  KlState* state = klstate_create(klmm, global, common, strpool, mapnodepool, NULL);
  if (!state) {
    klmm_restartgc(klmm);
    klmapnodepool_unpin(mapnodepool);
    klcommon_unpin(common, klmm);
    return NULL;
  }
  klmm_register_root(klmm, klmm_to_gcobj(state));
  klmm_restartgc(klmm);
  klmapnodepool_unpin(mapnodepool);
  klcommon_unpin(common, klmm);
  return state;
}

