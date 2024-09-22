#include "include/klapi.h"
#include "include/lang/klconfig.h"
#include "include/lang/klconvert.h"
#include "include/misc/klutils.h"
#include "include/value/klstate.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

static KlException kllib_cast_toint(KlState* state);
static KlException kllib_cast_tofloat(KlState* state);
static KlException kllib_cast_tonumber(KlState* state);
static KlException kllib_cast_tostring(KlState* state);
static KlException kllib_cast_tobool(KlState* state);
static KlException kllib_cast_createclass(KlState* state);



KlException KLCONFIG_LIBRARY_CAST_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring(state, "cast"));
  KLAPI_PROTECT(kllib_cast_createclass(state));
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  return klapi_return(state, 0);
}

static KlException kllib_cast_toint(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  KlValue* value = klapi_access(state, -1);
  switch (klvalue_gettype(value)) {
    case KL_STRING: {
      KlString* str = klvalue_getobj(value, KlString*);
      const char* cstr = klstring_content(str);
      size_t len = klstring_length(str);
      char* endptr;
      KlInt result = kllang_str2int(cstr, &endptr, 0);
      if (kl_unlikely(endptr != cstr + len))
        return klapi_throw_internal(state, KL_E_INVLD, "not a valid integer string");
      klapi_setint(state, -1, result);
      return klapi_return(state, 1);
    }
    case KL_FLOAT: {
      klapi_setint(state, -1, klcast(KlInt, klvalue_getfloat(value)));
      return klapi_return(state, 1);
    }
    default: {
      return klapi_throw_internal(state, KL_E_INVLD, "can not cast type '%s' to integer",
                                  klapi_typename(state, value));
    }
  }
}

static KlException kllib_cast_tofloat(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  KlValue* value = klapi_access(state, -1);
  switch (klvalue_gettype(value)) {
    case KL_STRING: {
      KlString* str = klvalue_getobj(value, KlString*);
      const char* cstr = klstring_content(str);
      size_t len = klstring_length(str);
      char* endptr;
      KlFloat result = kllang_str2float(cstr, &endptr);
      if (kl_unlikely(endptr != cstr + len))
        return klapi_throw_internal(state, KL_E_INVLD, "not a valid floating number string");
      klapi_setfloat(state, -1, result);
      return klapi_return(state, 1);
    }
    case KL_INT: {
      klapi_setfloat(state, -1, klcast(KlFloat, klvalue_getint(value)));
      return klapi_return(state, 1);
    }
    default: {
      return klapi_throw_internal(state, KL_E_INVLD, "can not cast type '%s' to floating number",
                                  klapi_typename(state, value));
    }
  }
}

static KlException kllib_cast_tonumber(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  KlValue* value = klapi_access(state, -1);
  switch (klvalue_gettype(value)) {
    case KL_STRING: {
      KlString* str = klvalue_getobj(value, KlString*);
      const char* cstr = klstring_content(str);
      size_t len = klstring_length(str);
      char* endptr;
      /* first try to cast to integer */
      KlInt intres = kllang_str2int(cstr, &endptr, 0);
      if (endptr == cstr + len) { /* success */
        klapi_setint(state, -1, intres);
        return klapi_return(state, 1);
      }
      /* else try to cast to floating number */
      KlFloat floatres = kllang_str2float(cstr, &endptr);
      if (kl_unlikely(endptr != cstr + len))
        return klapi_throw_internal(state, KL_E_INVLD, "not a valid number string");
      klapi_setfloat(state, -1, floatres);
      return klapi_return(state, 1);
    }
    case KL_FLOAT:
    case KL_INT: {
      return klapi_return(state, 1);
    }
    default: {
      return klapi_throw_internal(state, KL_E_INVLD, "can not cast type '%s' to number",
                                  klapi_typename(state, value));
    }
  }
}

static KlException kllib_cast_tostring(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  KlValue* value = klapi_access(state, -1);
  switch (klvalue_gettype(value)) {
    case KL_STRING: {
      return klapi_return(state, 1);
    }
    case KL_INT: {
      char buf[sizeof (KlInt) * CHAR_BIT];
      kllang_int2str(buf, sizeof (KlInt) * CHAR_BIT, klvalue_getint(value));
      KLAPI_PROTECT(klapi_setstring(state, -1, buf));
      return klapi_return(state, 1);
    }
    case KL_FLOAT: {
      char buf[200];
      kllang_float2str(buf, sizeof (buf) / sizeof (buf[0]), klvalue_getfloat(value));
      KLAPI_PROTECT(klapi_setstring(state, -1, buf));
      return klapi_return(state, 1);
    }
    case KL_BOOL: {
      KLAPI_PROTECT(klapi_setstring(state, -1, klvalue_getbool(value) == KL_TRUE ? "true" : "false"));
      return klapi_return(state, 1);
    }
    case KL_NIL: {
      KLAPI_PROTECT(klapi_setstring(state, -1, "nil"));
      return klapi_return(state, 1);
    }
    default: {
      return klapi_throw_internal(state, KL_E_INVLD, "can not cast type '%s' to string",
                                  klapi_typename(state, value));
    }
  }
}

static KlException kllib_cast_tobool(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  KlValue* value = klapi_access(state, -1);
  if ((klvalue_checktype(value, KL_BOOL) && klvalue_getbool(value) == KL_FALSE) ||
       klvalue_checktype(value, KL_NIL)) {
    klapi_setbool(state, -1, KL_FALSE);
  } else {
    klapi_setbool(state, -1, KL_TRUE);
  }
  return klapi_return(state, 1);
}

static KlException kllib_cast_createclass(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  KlClass* castclass = klclass_create(klstate_getmm(state), 1, KLOBJECT_DEFAULT_ATTROFF, NULL, NULL);
  if (kl_unlikely(!castclass))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating cast");
  klapi_pushobj(state, castclass, KL_CLASS);

  KLAPI_PROTECT(klapi_pushstring(state, "toint"));
  klapi_pushcfunc(state, kllib_cast_toint);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "tofloat"));
  klapi_setcfunc(state, -1, kllib_cast_tofloat);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "tonumber"));
  klapi_setcfunc(state, -1, kllib_cast_tonumber);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "tostring"));
  klapi_setcfunc(state, -1, kllib_cast_tostring);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "tobool"));
  klapi_setcfunc(state, -1, kllib_cast_tobool);
  KLAPI_PROTECT(klapi_class_newshared_normal(state, castclass, klapi_getstring(state, -2)));

  klapi_pop(state, 2);
  return KL_E_NONE;
}
