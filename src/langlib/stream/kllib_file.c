#include "include/langlib/stream/kllib_file.h"
#include "include/klapi.h"
#include "include/misc/klutils.h"
#include "include/value/klclass.h"
#include "include/value/klstate.h"
#include "include/value/klstring.h"
#include "include/vm/klexception.h"
#include "deps/k/include/kio/kifile.h"
#include "deps/k/include/kio/kofile.h"


static KlException kllib_ifile_init(KlState* state);
static KlException kllib_ofile_init(KlState* state);

KlException kllib_ifile_createclass(KlState* state, KlClass* istream) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  kl_assert(!klclass_isfinal(istream), "");
  KlClass* ifile = klclass_inherit(klstate_getmm(state), istream);
  if (kl_unlikely(!ifile))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating ifile class");
  klapi_pushobj(state, ifile, KL_CLASS);
  KLAPI_PROTECT(klapi_pushstring(state, "init"));
  KLAPI_PROTECT(klclass_newshared_method(ifile, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ifile_init)));
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
  KLAPI_PROTECT(klapi_pushstring(state, "init"));
  KLAPI_PROTECT(klclass_newshared_method(ofile, klstate_getmm(state), klapi_getstring(state, -1), &klvalue_cfunc(kllib_ofile_init)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

KlException kllib_ifile_createstdin(KlState* state, KlClass* ifile) {
  KLAPI_PROTECT(klapi_class_newobject(state, ifile));
  KlInputStream* ifile_stdin = klapi_getobj(state, -1, KlInputStream*);
  Ki* ki = kifile_attach_stdin(stdin);
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating stdin");
  kllib_istream_set(ifile_stdin, ki, NULL);
  kllib_istream_clroption(ifile_stdin, KLLIB_ISTREAM_FBUF);
  return KL_E_NONE;
}

KlException kllib_ofile_createstdout(KlState* state, KlClass* ofile) {
  KLAPI_PROTECT(klapi_class_newobject(state, ofile));
  KlOutputStream* ofile_stdout = klapi_getobj(state, -1, KlOutputStream*);
  Ko* ko = kofile_attach(stdout);
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating stdout");
  kllib_ostream_set(ofile_stdout, ko, NULL);
  kllib_ostream_clroption(ofile_stdout, KLLIB_OSTREAM_FBUF);
  return KL_E_NONE;
}

KlException kllib_ofile_createstderr(KlState* state, KlClass* ofile) {
  KLAPI_PROTECT(klapi_class_newobject(state, ofile));
  KlOutputStream* ofile_stderr = klapi_getobj(state, -1, KlOutputStream*);
  Ko* ko = kofile_attach(stderr);
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating stderr");
  kllib_ostream_set(ofile_stderr, ko, NULL);
  kllib_ostream_clroption(ofile_stderr, KLLIB_OSTREAM_FBUF);
  return KL_E_NONE;
}

static KlException kllib_ifile_init(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_istream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with istream");
  if (!klapi_checkstringb(state, 1))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide a file path");

  KlString* filepath = klapi_getstringb(state, 1);
  Ki* ki = kifile_create(klstring_content(filepath), "rb");
  if (kl_unlikely(!ki))
    return klapi_throw_internal(state, KL_E_INVLD, "failed to open file: %.*s", klstring_length(filepath), klstring_content(filepath));
  kllib_istream_set(klapi_getobjb(state, 0, KlInputStream*), ki, NULL);
  return klapi_return(state, 0);
}

static KlException kllib_ofile_init(KlState* state) {
  if (klapi_narg(state) < 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "missing arguments");
  if (!kllib_ostream_compatible(klapi_accessb(state, 0)))
    return klapi_throw_internal(state, KL_E_INVLD, "please call with ostream");
  if (!klapi_checkstringb(state, 1))
    return klapi_throw_internal(state, KL_E_INVLD, "please provide a file path");

  KlString* filepath = klapi_getstringb(state, 1);
  Ko* ko = kofile_create(klstring_content(filepath), "wb");
  if (kl_unlikely(!ko))
    return klapi_throw_internal(state, KL_E_INVLD, "failed to open file: %.*s", klstring_length(filepath), klstring_content(filepath));
  kllib_ostream_set(klapi_getobjb(state, 0, KlOutputStream*), ko, NULL);
  return klapi_return(state, 0);
}
