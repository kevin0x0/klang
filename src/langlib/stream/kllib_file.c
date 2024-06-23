#include "include/kio/kifile.h"
#include "include/kio/kofile.h"
#include "include/klapi.h"
#include "include/langlib/stream/kllib_stream.h"
#include "include/value/klclass.h"
#include "include/value/klstate.h"
#include "include/value/klstring.h"
#include "include/vm/klexception.h"


static KlException kllib_ifile_constructor(KlState* state);
static KlException kllib_ofile_constructor(KlState* state);

KlException kllib_ifile_createclass(KlState* state, KlClass* istream) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  kl_assert(!klclass_isfinal(istream), "");
  KlClass* ifile = klclass_inherit(klstate_getmm(state), istream);
  if (kl_unlikely(!ifile))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating ifile class");
  klapi_pushobj(state, ifile, KL_CLASS);
  KLAPI_PROTECT(klapi_pushstring(state, "constructor"));
  KLAPI_PROTECT(klclass_newfield(ifile, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ifile_constructor)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

KlException kllib_ofile_createclass(KlState* state, KlClass* ostream) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  kl_assert(!klclass_isfinal(ostream), "");
  KlClass* ofile = klclass_inherit(klstate_getmm(state), ostream);
  if (kl_unlikely(!ofile))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating ofile class");
  klapi_pushobj(state, ofile, KL_CLASS);
  KLAPI_PROTECT(klapi_pushstring(state, "constructor"));
  KLAPI_PROTECT(klclass_newfield(ofile, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ofile_constructor)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

static KlException kllib_ifile_constructor(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_istream_compatiable(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with istream");
  if (!klapi_checktypeb(state, 1, KL_STRING))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide a file path");

  KlString* filepath = klapi_getstringb(state, 1);
  Ki* ki = kifile_create(klstring_content(filepath), "rb");
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_INVLD, "failed to open file: %.*s", klstring_length(filepath), klstring_content(filepath));
  kllib_istream_set(klapi_getobjb(state, 0, KlInputStream*), ki);
  return klapi_return(state, 0);
}

static KlException kllib_ofile_constructor(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_ostream_compatiable(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostream");
  if (!klapi_checktypeb(state, 1, KL_STRING))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide a file path");

  KlString* filepath = klapi_getstringb(state, 1);
  Ko* ko = kofile_create(klstring_content(filepath), "w");
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_INVLD, "failed to open file: %.*s", klstring_length(filepath), klstring_content(filepath));
  kllib_ostream_set(klapi_getobjb(state, 0, KlOutputStream*), ko);
  return klapi_return(state, 0);
}
