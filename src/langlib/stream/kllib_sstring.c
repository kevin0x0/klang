#include "include/kio/ko.h"
#include "include/klapi.h"
#include "include/langlib/stream/kllib_kistring.h"
#include "include/langlib/stream/kllib_kostring.h"
#include "include/langlib/stream/kllib_strbuf.h"
#include "include/langlib/stream/kllib_stream.h"
#include "include/misc/klutils.h"
#include "include/value/klclass.h"
#include "include/value/klstate.h"
#include "include/vm/klexception.h"


static KlException kllib_istring_init(KlState* state);
static KlException kllib_ostring_init(KlState* state);
static KlException kllib_ostring_tostring(KlState* state);



KlException kllib_istring_createclass(KlState* state, KlClass* istream) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  kl_assert(!klclass_isfinal(istream), "");
  KlClass* istring = klclass_inherit(klstate_getmm(state), istream);
  if (kl_unlikely(!istring))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating istring class");
  klapi_pushobj(state, istring, KL_CLASS);
  KLAPI_PROTECT(klapi_pushstring(state, "init"));
  KLAPI_PROTECT(klclass_newshared_method(istring, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_istring_init)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

KlException kllib_ostring_createclass(KlState* state, KlClass* ostream) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  kl_assert(!klclass_isfinal(ostream), "");
  KlClass* ostring = klclass_inherit(klstate_getmm(state), ostream);
  if (kl_unlikely(!ostring))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating istring class");
  klapi_pushobj(state, ostring, KL_CLASS);
  KLAPI_PROTECT(klapi_pushstring(state, "init"));
  KLAPI_PROTECT(klclass_newshared_method(ostring, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ostring_init)));
  KLAPI_PROTECT(klapi_setstring(state, -1, "tostring"));
  KLAPI_PROTECT(klclass_newshared_method(ostring, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ostring_tostring)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

static KlException kllib_istring_init(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_istream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with istream");
  if (!klapi_checktypeb(state, 1, KL_STRING))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide a string");
  KlInputStream* istring = klapi_getobjb(state, 0, KlInputStream*);
  Ki* ki = kistring_create(klapi_getstringb(state, 1));
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating input stream");
  kllib_istream_set(istring, ki, (KiProp)kistring_prop);
  return klapi_return(state, 0);
}

static KlException kllib_ostring_init(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_ostream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostream");
  if (!klapi_checktypeb(state, 1, KL_INT))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide an initial buffer size");
  KlOutputStream* ostring = klapi_getobjb(state, 0, KlOutputStream*);
  Ko* ko = kostring_create(klapi_getintb(state, 1));
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating output stream");
  kllib_ostream_set(ostring, ko, NULL);
  return klapi_return(state, 0);
}

static KlException kllib_ostring_tostring(KlState* state) {
  if (klapi_narg(state) < 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_ostream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostring object");
  KlOutputStream* ostring = klapi_getobjb(state, 0, KlOutputStream*);
  Ko* ko = kllib_ostream_getko(ostring);
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_INVLD, "uninitialzed ostream object");
  if (kl_unlikely(!kostring_compatible(ko)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostring object");
  ko_flush(ko);
  KlStringBuf* strbuf = kostring_getstrbuf(klcast(KoString*, ko));
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring_buf(state, klstrbuf_access(strbuf, 0), klstrbuf_size(strbuf)));
  return klapi_return(state, 1);
}
