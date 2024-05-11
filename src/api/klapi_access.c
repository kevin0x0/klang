#include "include/klapi.h"
#include "include/lang/klconvert.h"

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

