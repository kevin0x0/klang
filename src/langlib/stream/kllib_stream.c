#include "include/langlib/stream/kllib_stream.h"
#include "include/langlib/stream/kllib_strbuf.h"
#include "include/langlib/stream/kllib_write.h"
#include "include/klapi.h"
#include "include/value/klclass.h"
#include "include/value/klstate.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include "include/mm/klmm.h"
#include "include/misc/klutils.h"
#include "deps/k/include/array/kgarray.h"
#include "deps/k/include/kio/ki.h"
#include "deps/k/include/kio/ko.h"

kgarray_impl(char, KlStringBuf, klstrbuf, pass_val, nonstatic);

static KlGCObject* kllib_istream_prop(KlInputStream* istream, KlMM* klmm, KlGCObject* gclist);
static KlGCObject* kllib_ostream_prop(KlOutputStream* ostream, KlMM* klmm, KlGCObject* gclist);
static void kllib_istream_delete(KlInputStream* istream, KlMM* klmm);
static void kllib_ostream_delete(KlOutputStream* ostream, KlMM* klmm);


KlGCVirtualFunc kllib_istream_gcvfunc = { .propagate = (KlGCProp)kllib_istream_prop, .after = NULL, .destructor = (KlGCDestructor)kllib_istream_delete };
KlGCVirtualFunc kllib_ostream_gcvfunc = { .propagate = (KlGCProp)kllib_ostream_prop, .after = NULL, .destructor = (KlGCDestructor)kllib_ostream_delete };


static KlException kllib_istream_objconstructor(KlClass* klclass, KlMM* klmm, KlValue* result);
static KlException kllib_ostream_objconstructor(KlClass* klclass, KlMM* klmm, KlValue* result);


static KlException kllib_istream_readline(KlState* state);
static KlException kllib_ostream_writeline(KlState* state);
static KlException kllib_istream_close(KlState* state);
static KlException kllib_ostream_close(KlState* state);

KlException kllib_istream_createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KlMM* klmm = klstate_getmm(state);
  KlClass* istreambase = klclass_create(klmm, 3, klobject_attrarrayoffset(KlInputStream), NULL, kllib_istream_objconstructor);
  if (kl_unlikely(!istreambase))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating class for istreambase");
  klapi_pushobj(state, istreambase, KL_CLASS);

  /* set virtual method for istreambase */
  KlValue nil = KLVALUE_NIL_INIT;
  KLAPI_PROTECT(klapi_pushstring(state, "get"));
  KLAPI_PROTECT(klclass_newshared_method(istreambase, klmm, klapi_getstring(state, -1), &nil));
  KLAPI_PROTECT(klapi_setstring(state, -1, "read"));
  KLAPI_PROTECT(klclass_newshared_method(istreambase, klmm, klapi_getstring(state, -1), &nil));
  KLAPI_PROTECT(klapi_setstring(state, -1, "readline"));
  KLAPI_PROTECT(klclass_newshared_method(istreambase, klmm, klapi_getstring(state, -1), &klvalue_cfunc(kllib_istream_readline)));
  KLAPI_PROTECT(klapi_setstring(state, -1, "close"));
  KLAPI_PROTECT(klclass_newshared_method(istreambase, klmm, klapi_getstring(state, -1), &klvalue_cfunc(kllib_istream_close)));
  klapi_pop(state, 1); /* pop string */
  return KL_E_NONE;
}

KlException kllib_ostream_createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KlMM* klmm = klstate_getmm(state);
  KlClass* ostreambase = klclass_create(klmm, 3, klobject_attrarrayoffset(KlInputStream), NULL, kllib_ostream_objconstructor);
  if (kl_unlikely(!ostreambase))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating class for istreambase");
  klapi_pushobj(state, ostreambase, KL_CLASS);

  /* set virtual method for ostreambase */
  KlValue nil = KLVALUE_NIL_INIT;
  KLAPI_PROTECT(klapi_pushstring(state, "put"));
  KLAPI_PROTECT(klclass_newshared_method(ostreambase, klmm, klapi_getstring(state, -1), &nil));
  KLAPI_PROTECT(klapi_setstring(state, -1, "write"));
  KLAPI_PROTECT(klclass_newshared_method(ostreambase, klmm, klapi_getstring(state, -1), &klvalue_cfunc(kllib_ostream_write)));
  KLAPI_PROTECT(klapi_setstring(state, -1, "writeline"));
  KLAPI_PROTECT(klclass_newshared_method(ostreambase, klmm, klapi_getstring(state, -1), &klvalue_cfunc(kllib_ostream_writeline)));
  KLAPI_PROTECT(klapi_setstring(state, -1, "close"));
  KLAPI_PROTECT(klclass_newshared_method(ostreambase, klmm, klapi_getstring(state, -1), &klvalue_cfunc(kllib_ostream_close)));
  klapi_pop(state, 1); /* pop string */
  return KL_E_NONE;
}

bool kllib_istream_compatible(KlValue* val) {
  return klvalue_checktype(val, KL_OBJECT) && klobject_compatible(klvalue_getobj(val, KlObject*), kllib_istream_objconstructor);
}

bool kllib_ostream_compatible(KlValue* val) {
  return klvalue_checktype(val, KL_OBJECT) && klobject_compatible(klvalue_getobj(val, KlObject*), kllib_ostream_objconstructor);
}

Ki* kllib_istream_getki(KlInputStream* istream) {
  return istream->ki;
}

Ko* kllib_ostream_getko(KlOutputStream* ostream) {
  return ostream->ko;
}

void kllib_istream_set(KlInputStream* istream, Ki* ki, KiProp kiprop) {
  if (istream->ki) ki_delete(istream->ki);
  istream->ki = ki;
  istream->kiprop = kiprop;
}

void kllib_ostream_set(KlOutputStream* ostream, Ko* ko, KoProp koprop) {
  if (ostream->ko) ko_delete(ostream->ko);
  ostream->ko = ko;
  ostream->koprop = koprop;
}

static KlException kllib_istream_readline(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "please call with exactly 1 argument('this')!");
  if (!kllib_istream_compatible(klapi_access(state, -1)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with istream object!");
  Ki* ki = klapi_getobj(state, -1, KlInputStream*)->ki;
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_INVLD, "uninitialized istream object");

  KlStringBuf buf;
  if (kl_unlikely(!klstrbuf_init(&buf, 64)))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating string buffer");
  int ch;
  while ((ch = ki_getc(ki)) != KOF && ch != '\n' && ch != '\r') {
    KLAPI_MAYFAIL(klstrbuf_push_back(&buf, ch) ? KL_E_NONE :
                                                 klapi_throw_internal(state, KL_E_OOM, "out of memory while read a line"),
                  klstrbuf_destroy(&buf));
  }
  /* finish read nl */
  if (ch == '\r') {
    int tmpch = ki_getc(ki);
    if (tmpch != '\n' && tmpch != KOF)
      ki_ungetc(ki);
  }

  size_t len = klstrbuf_size(&buf); 
  if (len == 0 && ch == KOF) { /* reach the end of stream */
    klstrbuf_destroy(&buf);
    return klapi_return(state, 0);
  }
  KLAPI_MAYFAIL(klapi_pushstring_buf(state, klstrbuf_access(&buf, 0), klstrbuf_size(&buf)), klstrbuf_destroy(&buf));
  klstrbuf_destroy(&buf);
  return klapi_return(state, 1);
}

static KlException kllib_ostream_writeline(KlState* state) {
  if (klapi_narg(state) == 0)
    return klapi_throw_internal(state, KL_E_ARGNO, "please call with at least 1 argument(including 'this')!");
  if (!kllib_ostream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostream object!");
  KLAPI_PROTECT(klapi_scall(state, &klvalue_cfunc(kllib_ostream_write), klapi_narg(state), 1));
  KlOutputStream* ostream = klapi_getobj(state, -1, KlOutputStream*);
  Ko* ko = kllib_ostream_getko((ostream));
  ko_putc(ko, '\n');
  if (!kllib_ostream_testoption(ostream, KLLIB_OSTREAM_FBUF))
    ko_flush(ko);
  return klapi_return(state, 1);
}

static KlException kllib_istream_close(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "please call with exactly 1 argument('this')!");
  if (!kllib_istream_compatible(klapi_access(state, -1)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with istream object!");
  KlInputStream* istream = klapi_getobj(state, -1, KlInputStream*);
  if (istream->ki) {
    ki_delete(istream->ki);
    istream->ki = NULL;
  }
  return klapi_return(state, 0);
}

static KlException kllib_ostream_close(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "please call with exactly 1 argument('this')!");
  if (!kllib_ostream_compatible(klapi_access(state, -1)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostream object!");
  KlOutputStream* ostream = klapi_getobj(state, -1, KlOutputStream*);
  if (ostream->ko) {
    ko_delete(ostream->ko);
    ostream->ko = NULL;
  }
  return klapi_return(state, 0);
}

static KlException kllib_istream_objconstructor(KlClass* klclass, KlMM* klmm, KlValue* result) {
  KlInputStream* istream = klcast(KlInputStream*, klclass_objalloc(klclass, klmm));
  if (kl_unlikely(!istream)) return KL_E_OOM;
  istream->ki = NULL;
  istream->kiprop = NULL;
  istream->option = KLLIB_ISTREAM_FBUF;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(istream), &kllib_istream_gcvfunc);
  klvalue_setobj(result, istream, KL_OBJECT);
  return KL_E_NONE;
}

static KlException kllib_ostream_objconstructor(KlClass* klclass, KlMM* klmm, KlValue* result) {
  KlOutputStream* ostream = klcast(KlOutputStream*, klclass_objalloc(klclass, klmm));
  if (kl_unlikely(!ostream)) return KL_E_OOM;
  ostream->ko = NULL;
  ostream->koprop = NULL;
  ostream->option = KLLIB_OSTREAM_FBUF;
  klmm_gcobj_enable(klmm, klmm_to_gcobj(ostream), &kllib_ostream_gcvfunc);
  klvalue_setobj(result, ostream, KL_OBJECT);
  return KL_E_NONE;
}

static KlGCObject* kllib_istream_prop(KlInputStream* istream, KlMM* klmm, KlGCObject* gclist) {
  if (istream->kiprop) istream->kiprop(istream->ki, klmm, gclist);
  return klobject_propagate_nomm(klcast(KlObject*, istream), gclist);
}

static KlGCObject* kllib_ostream_prop(KlOutputStream* ostream, KlMM* klmm, KlGCObject* gclist) {
  if (ostream->koprop) ostream->koprop(ostream->ko, klmm, gclist);
  return klobject_propagate_nomm(klcast(KlObject*, ostream), gclist);
}

static void kllib_istream_delete(KlInputStream* istream, KlMM* klmm) {
  if (istream->ki) ki_delete(istream->ki);
  klobject_free(klcast(KlObject*, istream), klmm);
}

static void kllib_ostream_delete(KlOutputStream* ostream, KlMM* klmm) {
  if (ostream->ko) ko_delete(ostream->ko);
  klobject_free(klcast(KlObject*, ostream), klmm);
}

