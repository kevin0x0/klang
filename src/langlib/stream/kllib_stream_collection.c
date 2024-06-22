#include "include/klapi.h"
#include "include/langlib/stream/kllib_stream.h"
#include "include/langlib/stream/kllib_file.h"
#include "include/misc/klutils.h"
#include "include/value/klcfunc.h"
#include "include/value/klclass.h"
#include "include/vm/klexception.h"



static KlException kllib_create_istream_collection(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  KlClass* istreamcollection = klclass_create(klstate_getmm(state), 3, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!istreamcollection))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating istream collection");
  klclass_final(istreamcollection);
  klapi_pushobj(state, istreamcollection, KL_CLASS);

  KLAPI_PROTECT(kllib_istream_createclass(state));
  KLAPI_PROTECT(klapi_pushstring(state, "base"));
  KLAPI_PROTECT(klclass_newfield(istreamcollection, klstate_getmm(state), klapi_getstring(state, -1), klapi_access(state, -2)));

  KlClass* base = klapi_getobj(state, -2, KlClass*);
  klapi_pop(state, 2);

  KLAPI_PROTECT(kllib_ifile_createclass(state, base));
  KLAPI_PROTECT(klapi_pushstring(state, "file"));
  KLAPI_PROTECT(klclass_newfield(istreamcollection, klstate_getmm(state), klapi_getstring(state, -1), klapi_access(state, -2)));
  klapi_pop(state, 2);
  return KL_E_NONE;
}

static KlException kllib_create_ostream_collection(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  KlClass* ostreamcollection = klclass_create(klstate_getmm(state), 3, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!ostreamcollection))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating ostream collection");
  klclass_final(ostreamcollection);
  klapi_pushobj(state, ostreamcollection, KL_CLASS);

  KLAPI_PROTECT(kllib_ostream_createclass(state));
  KLAPI_PROTECT(klapi_pushstring(state, "base"));
  KLAPI_PROTECT(klclass_newfield(ostreamcollection, klstate_getmm(state), klapi_getstring(state, -1), klapi_access(state, -2)));

  KlClass* base = klapi_getobj(state, -2, KlClass*);
  klapi_pop(state, 2);

  KLAPI_PROTECT(kllib_ofile_createclass(state, base));
  KLAPI_PROTECT(klapi_pushstring(state, "file"));
  KLAPI_PROTECT(klclass_newfield(ostreamcollection, klstate_getmm(state), klapi_getstring(state, -1), klapi_access(state, -2)));
  klapi_pop(state, 2);
  return KL_E_NONE;
}

KlException kllib_init(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));

  KLAPI_PROTECT(kllib_create_istream_collection(state));
  KLAPI_PROTECT(klapi_pushstring(state, "istream"));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -1), -2));

  klapi_pop(state, 2);
  KLAPI_PROTECT(kllib_create_ostream_collection(state));
  KLAPI_PROTECT(klapi_pushstring(state, "ostream"));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -1), -2));
  return klapi_return(state, 0);
}
