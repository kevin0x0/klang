#ifndef _KLANG_INCLUDE_VM_KLAPI_H_
#define _KLANG_INCLUDE_VM_KLAPI_H_

#include "include/value/klcfunc.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "include/vm/klexec.h"
#include "include/value/klstate.h"
#include "include/value/klarray.h"
#include "include/misc/klutils.h"


#define KLAPI_VARIABLE_RESULTS    (KLEXEC_VARIABLE_RESULTS)


#define KLAPI_PROTECT(expr) do {                        \
  KlException _kl_exception = (expr);                   \
  if (kl_unlikely(_kl_exception)) return _kl_exception; \
} while (0)

#define KLAPI_MAYFAIL(expr, cleaner) do {               \
  KlException _kl_exception = (expr);                   \
  if (kl_unlikely(_kl_exception)) {                     \
    cleaner;                                            \
    return _kl_exception;                               \
  }                                                     \
} while (0)


#define klapi_pushobj(state, obj, type)               klapi_pushgcobj((state), (KlGCObject*)(obj), (type))
#define klapi_setobj(state, index, obj, type)         klapi_setgcobj((state), (index), (KlGCObject*)(obj), (type))
#define klapi_getobj(state, index, type)              klcast(type, klapi_getgcobj((state), (index)))
#define klapi_getobjb(state, index, type)             klcast(type, klapi_getgcobjb((state), (index)))

#define klapi_throw_internal(state, etype, ...)       klstate_throw((state), (etype), __VA_ARGS__)
#define klapi_throw(state, index)                     klthrow_user(&(state)->throwinfo, klapi_access((state), index))

#define klapi_currexception(state)                    klstate_currexception((state))
#define klapi_exception_message(state)                klstate_exception_message((state))
#define klapi_exception_source(state)                 klstate_exception_source((state))
#define klapi_exception_object(state)                 klstate_expcetion_object((state))

#define klapi_kfunc_initdone(state, kfunc)            klkfunc_initdone(klstate_getmm((state)), (kfunc))
#define klapi_kfunc_initabort(state, kfunc)           klkfunc_initabort(klstate_getmm((state)), (kfunc))
#define klapi_kfunc_insts(state, kfunc)               klkfunc_insts((kfunc))
#define klapi_kfunc_constants(state, kfunc)           klkfunc_constants((kfunc))
#define klapi_kfunc_refinfo(state, kfunc)             klkfunc_refinfo((kfunc))
#define klapi_kfunc_subfunc(state, kfunc)             klkfunc_subfunc((kfunc))
#define klapi_kfunc_posinfo(state, kfunc)             klkfunc_posinfo((kfunc))
#define klapi_kfunc_srcfile(state, kfunc)             klkfunc_srcfile((kfunc))
#define klapi_kfunc_entrypoint(state, kfunc)          klkfunc_entrypoint((kfunc))
#define klapi_kfunc_codelen(state, kfunc)             klkfunc_codelen((kfunc))
#define klapi_kfunc_framesize(state, kfunc)           klkfunc_framesize((kfunc))
#define klapi_kfunc_nparam(state, kfunc)              klkfunc_nparam((kfunc))
#define klapi_kfunc_setposinfo(state, kfunc, posinfo) klkfunc_setposinfo((kfunc), (posinfo))
#define klapi_kfunc_setsrcfile(state, kfunc, src)     klkfunc_setsrcfile((kfunc), (src))


#define klapi_checktype(state, index, type)           (klapi_gettype((state), (index)) == (type))
#define klapi_checktypeb(state, index, type)          (klapi_gettypeb((state), (index)) == (type))

/* number of arguments passed by caller */
#define klapi_narg(state)                             (klstate_currci(state)->narg)
/* expected number of returned values.
 * if is KLAPI_VARIABLE_RESULTS, then the caller wants all results. */
#define klapi_nres(state)                             (klstate_currci(state)->nret)
#define klapi_framesize(state)                        klcast(size_t, (klstate_stktop((state)) - klstate_currci((state))->base))

#define klapi_tointb(state, to, from)                 klapi_toint((state), klapi_framesize((state)) + (to), klapi_framesize(state) + (from))
#define klapi_tofloatb(state, to, from)               klapi_tofloat((state), klapi_framesize((state)) + (to), klapi_framesize((state)) + (from))
#define klapi_toboolb(state, index)                   klapi_tobool((state), klapi_framesize((state)) + (index))
#define klapi_tostringb(state, index)                 klapi_tostring((state), klapi_framesize((state)) + (index))

#define klapi_exception_clear(state, callinfo)        klstate_setcurrci((state), (callinfo))

KlState* klapi_new_state(KlMM* klmm);

KlException klapi_scall(KlState* state, KlValue* callable, size_t narg, size_t nret);
KlException klapi_tryscall(KlState* state, int errhandler, KlValue* callable, size_t narg, size_t nret);
KlException klapi_call(KlState* state, KlValue* callable, size_t narg, size_t nret, KlValue* respos);
KlException klapi_tailcall(KlState* state, KlValue* callable, size_t narg);
/* the exception handler should be pushed into stack immediately before argmuments */
KlException klapi_trycall(KlState* state, int errhandler, KlValue* callable, size_t narg, size_t nret, KlValue* respos);

KlException klapi_allocstack(KlState* state, size_t size);
KlException klapi_checkstack(KlState* state, size_t size);
KlException klapi_checkframe(KlState* state, size_t size);
KlException klapi_checkframeandset(KlState* state, size_t size);
bool klapi_checkrange(KlState* state, int index);

KlValue* klapi_stacktop(KlState* state);
KlValue* klapi_pointer(KlState* state, int index);
KlValue* klapi_access(KlState* state, int index);
/* access from frame base */
KlValue* klapi_accessb(KlState* state, unsigned index);
KlType klapi_gettype(KlState* state, int index);
KlType klapi_gettypeb(KlState* state, int index);
void klapi_setframesize(KlState* state, unsigned size);
void klapi_pop(KlState* state, size_t count);
void klapi_close(KlState* state, KlValue* bound);
void klapi_popclose(KlState* state, size_t count);

/* push method */
void klapi_pushcfunc(KlState* state, KlCFunction* cfunc);
void klapi_pushint(KlState* state, KlInt val);
void klapi_pushuint(KlState* state, KlUInt val);
void klapi_pushfloat(KlState* state, KlFloat val);
void klapi_pushnil(KlState* state, size_t count);
void klapi_pushbool(KlState* state, KlBool val);
void klapi_pushuserdata(KlState* state, void* ud);
void klapi_pushgcobj(KlState* state, KlGCObject* gcobj, KlType type);
void klapi_pushvalue(KlState* state, KlValue* val);
KlException klapi_pushstring(KlState* state, const char* str);
KlException klapi_pushstring_buf(KlState* state, const char* buf, size_t buflen);
KlException klapi_pushmap(KlState* state, size_t capacity);
KlException klapi_pusharray(KlState* state, size_t capacity);

/* set method */
void klapi_setcfunc(KlState* state, int index, KlCFunction* cfunc);
void klapi_setint(KlState* state, int index, KlInt val);
void klapi_setnil(KlState* state, int index);
void klapi_setbool(KlState* state, int index, KlBool val);
void klapi_setuserdata(KlState* state, int index, void* ud);
void klapi_setgcobj(KlState* state, int index, KlGCObject* gcobj, KlType type);
void klapi_setvalue(KlState* state, int index, KlValue* val);
KlException klapi_setstring(KlState* state, int index, const char* str);
KlException klapi_setmap(KlState* state, int index, size_t capacity);
KlException klapi_setarray(KlState* state, int index, size_t capacity);

/* get method */
KlCFunction* klapi_getcfunc(KlState* state, int index);
KlCFunction* klapi_getcfuncb(KlState* state, unsigned index);
KlInt klapi_getint(KlState* state, int index);
KlInt klapi_getintb(KlState* state, unsigned index);
KlFloat klapi_getfloat(KlState* state, int index);
KlFloat klapi_getfloatb(KlState* state, unsigned index);
KlBool klapi_getbool(KlState* state, int index);
KlBool klapi_getboolb(KlState* state, unsigned index);
KlString* klapi_getstring(KlState* state, int index);
KlString* klapi_getstringb(KlState* state, unsigned index);
KlMap* klapi_getmap(KlState* state, int index);
KlMap* klapi_getmapb(KlState* state, unsigned index);
KlArray* klapi_getarray(KlState* state, int index);
KlArray* klapi_getarrayb(KlState* state, unsigned index);
KlGCObject* klapi_getgcobj(KlState* state, int index);
KlGCObject* klapi_getgcobjb(KlState* state, unsigned index);
void* klapi_getuserdata(KlState* state, int index);
void* klapi_getuserdatab(KlState* state, unsigned index);

/* to method */
KlException klapi_toint(KlState* state, int to, int from);
KlException klapi_tofloat(KlState* state, int to, int from);
KlBool klapi_tobool(KlState* state, int index);
KlString* klapi_tostring(KlState* state, int index);

/* access to global variable */
void klapi_loadglobal(KlState* state);
KlException klapi_storeglobal(KlState* state, KlString* varname, int validx);

/* operation on value */
KlKFunction* klapi_kfunc_alloc(KlState* state, unsigned codelen, unsigned short nconst,
                              unsigned short nref, unsigned short nsubfunc, KlUByte framesize, KlUByte nparam);

/* operator */
KlException klapi_concat(KlState* state);
KlException klapi_concati(KlState* state, int result, int left, int right);

/* auxiliary function for library */

/* C function */

/* return results.
 * to be returned values should be placed on top of the stack.
 */
KlException klapi_return(KlState* state, size_t nret);
KlException klapi_mkcclosure(KlState* state, int index, KlCFunction* cfunc, size_t nref);

/* access references */
KlValue* klapi_getref(KlState* state, unsigned short refidx);

/* library loader */

/* load a library by given 'libpath',
 * call the 'entryfunction'.
 * If 'entryfunction' is NULL, a default entry name "kllib_entrypoint" would be used.
 */
KlException klapi_loadlib(KlState* state, int result, const char* entryfunction);

KlException klapi_class_newshared_normal(KlState* state, KlClass* klclass, KlString* fieldname);
KlException klapi_class_newshared_method(KlState* state, KlClass* klclass, KlString* fieldname);
KlException klapi_class_newlocal(KlState* state, KlClass* klclass, KlString* fieldname);


#endif
