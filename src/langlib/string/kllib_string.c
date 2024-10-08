#include "include/klapi.h"
#include "include/lang/klconfig.h"
#include "include/misc/klutils.h"
#include "include/value/klstate.h"
#include "include/value/klstring.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include <string.h>


static KlException kllib_string_join_raw(KlState* state, size_t nval, KlValue* vals, KlValue* result);
static KlException kllib_string_slice(KlState* state);
static KlException kllib_string_find(KlState* state);
static KlException kllib_string_join(KlState* state);
static KlException kllib_string_split(KlState* state);

static KlException kllib_string_utf8idx(KlState* state);
static KlException kllib_string_utf8len(KlState* state);

static inline size_t kllib_string_utf8charlen(const unsigned char* str) {
  unsigned char ch = *str;
  if (ch < 0xC0) return 1;
  if (ch < 0xE0) return 2;
  if (ch < 0xF0) return 3;
  return 4;
}

static inline KlInt kllib_normalise_stringidx(KlInt idx, size_t strlength) {
  while (idx < 0)
    idx = strlength + idx;
  if (idx > klcast(KlInt, strlength))
    idx = idx % strlength;
  return idx;
}


KlException KLCONFIG_LIBRARY_STRING_ENTRYFUNCNAME(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KlClass* strclass = klstate_common(state)->klclass.phony[KL_STRING];

  KLAPI_PROTECT(klapi_pushstring(state, "find"));
  klapi_pushcfunc(state, kllib_string_find);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "join"));
  klapi_setcfunc(state, -1, kllib_string_join);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "slice"));
  klapi_setcfunc(state, -1, kllib_string_slice);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "split"));
  klapi_setcfunc(state, -1, kllib_string_split);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "utf8len"));
  klapi_setcfunc(state, -1, kllib_string_utf8len);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -2, "utf8idx"));
  klapi_setcfunc(state, -1, kllib_string_utf8idx);
  KLAPI_PROTECT(klapi_class_newshared_method(state, strclass, klapi_getstring(state, -2)));

  return klapi_return(state, 0);
}

static KlException kllib_string_slice(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 2 && klapi_narg(state) != 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected two or three arguments");
  if (kl_unlikely(!klapi_checkstringb(state, 0)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string, got %s", klstring_content(klapi_typename(state, klapi_accessb(state, 0))));
  KlString* str = klapi_getstringb(state, 0);
  size_t strlength = klstring_length(str);
  if (kl_unlikely(!klapi_checktypeb(state, 1, KL_INT)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected integer, got %s", klstring_content(klapi_typename(state, klapi_accessb(state, 1))));
  KlInt begin = kllib_normalise_stringidx(klapi_getintb(state, 1), strlength);
  KlInt end = strlength;
  if (klapi_narg(state) == 3) {
    if (kl_unlikely(!klapi_checktype(state, -1, KL_INT)))
      return klapi_throw_internal(state, KL_E_TYPE, "expected integer, got %s", klstring_content(klapi_typename(state, klapi_access(state, -1))));
    end = kllib_normalise_stringidx(klapi_getint(state, -1), strlength);
  }
  if (kl_unlikely(end < begin))
      return klapi_throw_internal(state, KL_E_RANGE, "invalid range: (%zd, %zd) for string: %s", begin, end, klstring_content(str));
  KlString* res = klstrpool_new_string_buf(klstate_strpool(state), klstring_content(str) + begin, end - begin);
  if (kl_unlikely(!res))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating string");
  klapi_setobj(state, -1, res, klvalue_getstringtype(res));
  return klapi_return(state, 1);
}

static KlException kllib_string_find(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 2 && klapi_narg(state) != 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected two or three arguments");
  KlInt beginidx = 0;
  if (klapi_narg(state) == 3) {
    if (kl_unlikely(!klapi_checktype(state, -1, KL_INT)))
      return klapi_throw_internal(state, KL_E_TYPE,
                                  "expected integer as third argument, got %s",
                                  klstring_content(klapi_typename(state, klapi_access(state, -1))));
    beginidx = klapi_getint(state, -1);
    klapi_pop(state, 1);
  }
  if (kl_unlikely(!klapi_checkstring(state, -2) || !klapi_checkstring(state, -1)))
    return klapi_throw_internal(state, KL_E_TYPE,
                                "expected two strings, got %s, %s",
                                klstring_content(klapi_typename(state, klapi_access(state, -2))),
                                klstring_content(klapi_typename(state, klapi_access(state, -1))));
  KlString* textklstr = klapi_getstring(state, -2);
  beginidx = kllib_normalise_stringidx(beginidx, klstring_length(textklstr));
  const char* text = klstring_content(textklstr);
  const char* pattern = klstring_content(klapi_getstring(state, -1));
  const char* str = strstr(text + beginidx, pattern);
  str ? klapi_setint(state, -1, str - text) : klapi_setnil(state, -1);
  return klapi_return(state, 1);
}

static KlException kllib_string_join(KlState* state) {
  KLAPI_PROTECT(klapi_checkframe(state, 1));
  size_t nstr = klapi_narg(state);
  if (nstr == 0) {
    KLAPI_PROTECT(klapi_pushstring(state, ""));
    return klapi_return(state, 1);
  }

  KlValue* strs = klapi_pointer(state, -nstr);
  KLAPI_PROTECT(kllib_string_join_raw(state, nstr, strs, strs + nstr - 1));
  return klapi_return(state, 1);
}

static KlException kllib_string_split(KlState* state) {
  if (klapi_narg(state) != 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected two arguments");
  if (!klapi_checkstring(state, -2) || !klapi_checkstring(state, -1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected two strings, got %s, %s", 
                                klstring_content(klapi_typename(state, klapi_access(state, -2))),
                                klstring_content(klapi_typename(state, klapi_access(state, -1))));
  KlString* self = klapi_getstring(state, -2);
  KlString* delim = klapi_getstring(state, -1);
  const char* self_raw = klstring_content(self);
  const char* delim_raw = klstring_content(delim);
  size_t selflen = klstring_length(self);
  size_t delimlen = klstring_length(delim);
  if (kl_unlikely(delimlen == 0))
    return klapi_throw_internal(state, KL_E_INVLD, "delimiter is empty string");
  const char* begin = self_raw;
  const char* end;
  size_t nret = 0;
  while ((end = strstr(begin, delim_raw))) {
    ++nret;
    KLAPI_PROTECT(klapi_checkstack(state, 1));
    KLAPI_PROTECT(klapi_pushstring_buf(state, begin, end - begin));
    begin = end + delimlen;
  }
  ++nret;
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  KLAPI_PROTECT(klapi_pushstring_buf(state, begin, self_raw + selflen - begin));
  return klapi_return(state, nret);
}

static KlException kllib_string_join_raw(KlState* state, size_t nval, KlValue* vals, KlValue* result) {
  size_t totallen = 0;
  for (KlValue* val = vals; val != vals + nval; ++val) {
    if (kl_unlikely(!klvalue_isstring(val)))
      return klapi_throw_internal(state, KL_E_TYPE, "expected string(s), got an %s", klstring_content(klapi_typename(state, val)));
    totallen += klstring_length(klvalue_getobj(val, KlString*));
  }
  switch (nval) {
    case 0: {
      KlString* str = klstrpool_new_string(klstate_strpool(state), "");
      if (kl_unlikely(!str))
        return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating string");
      klvalue_setstring(result, str);
      return KL_E_NONE;
    }
    case 1: {
      klvalue_setvalue(result, vals);
      return KL_E_NONE;
    }
    case 2: {
      KlString* str = klstrpool_string_concat(klstate_strpool(state), klvalue_getobj(vals, KlString*), klvalue_getobj(vals + 1, KlString*));
      if (kl_unlikely(!str))
        return klapi_throw_internal(state, KL_E_OOM, "out of memory while concatenating strings");
      klvalue_setstring(result, str);
      return KL_E_NONE;
    }
    default: {
      KlMM* klmm = klstate_getmm(state);
      char* tmpbuf = (char*)klmm_alloc(klmm, totallen * sizeof (char));
      if (kl_unlikely(!tmpbuf)) 
        return klapi_throw_internal(state, KL_E_OOM, "out of memory while allocate temporary buffer");
      char* currpos = tmpbuf;
      for (KlValue* val = vals; val != vals + nval; ++val) {
        KlString* str = klvalue_getobj(val, KlString*);
        size_t len = klstring_length(str);
        memcpy(currpos, klstring_content(str), len);
        currpos += len;
      }
      KlString* str = klstrpool_new_string_buf(klstate_strpool(state), tmpbuf, totallen);
      klmm_free(klmm, tmpbuf, totallen * sizeof (char));
      if (kl_unlikely(!str))
        return klapi_throw_internal(state, KL_E_OOM, "out of memory while creating string");
      klvalue_setstring(result, str);
      return KL_E_NONE;
    }
  }
}

static KlException kllib_string_utf8idx(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 2 && klapi_narg(state) != 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected two or three arguments");
  if (kl_unlikely(!klapi_checkstringb(state, 0)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string, got %s", klstring_content(klapi_typename(state, klapi_accessb(state, 0))));
  if (kl_unlikely(!klapi_checktypeb(state, 1, KL_INT)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected integer, got %s", klstring_content(klapi_typename(state, klapi_accessb(state, 1))));
  KlString* str = klapi_getstringb(state, 0);
  KlInt count = klapi_getintb(state, 1);
  KlInt begin = 0;
  if (klapi_narg(state) == 3) {
    if (kl_unlikely(!klapi_checktype(state, -1, KL_INT)))
      return klapi_throw_internal(state, KL_E_TYPE, "expected integer, got %s", klstring_content(klapi_typename(state, klapi_access(state, -1))));
    begin = kllib_normalise_stringidx(klapi_getint(state, -1), klstring_length(str));
  }
  if (kl_unlikely(count < 0))
    return klapi_throw_internal(state, KL_E_TYPE, "expected positive index");

  const unsigned char* text = (unsigned char*)klstring_content(str);
  size_t strlength = klstring_length(str);
  size_t idx = begin;
  while (count-- > 0) {
    idx += kllib_string_utf8charlen(text + idx);
    if (kl_unlikely(idx >= strlength))
      return klapi_throw_internal(state, KL_E_RANGE, "there are not that many utf8 characters");
  }

  klapi_setint(state, -1, idx);
  return klapi_return(state, 1);
}

static KlException kllib_string_utf8len(KlState* state) {
  if (kl_unlikely(klapi_narg(state) != 1))
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argument");
  if (kl_unlikely(!klapi_checkstring(state, -1)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string, got %s", klstring_content(klapi_typename(state, klapi_accessb(state, 0))));
  KlString* str = klapi_getstring(state, -1);
  const unsigned char* text = (unsigned char*)klstring_content(str); 
  size_t strlength = klstring_length(str);
  size_t idx = 0;
  KlInt count = 0;
  while (idx < strlength) {
    idx += kllib_string_utf8charlen(text + idx);
    ++count;
  }
  klapi_setint(state, -1, count);
  return klapi_return(state, 1);
}
