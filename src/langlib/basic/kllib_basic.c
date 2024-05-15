#include "include/klapi.h"
#include "include/value/klarray.h"
#include "include/value/klmap.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include <stdio.h>

static KlException kllib_basic_print(KlState* state);
static KlException kllib_basic_map_next(KlState* state);
static KlException kllib_basic_arr_next(KlState* state);
static KlException kllib_basic_map_iter(KlState* state);
static KlException kllib_basic_arr_iter(KlState* state);

KlException kllib_init(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KLAPI_PROTECT(klapi_pushstring(state, "print"));
  klapi_pushcfunc(state, kllib_basic_print);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  klapi_pop(state, 2);
  klapi_pushcfunc(state, kllib_basic_map_iter);
  KlException exception = klclass_newshared(state->common->klclass.map, klstate_getmm(state), state->common->string.iter, klapi_access(state, -1));
  if (kl_unlikely(exception))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while setting shared field");
  klapi_setcfunc(state, -1, kllib_basic_arr_iter);
  exception = klclass_newshared(state->common->klclass.array, klstate_getmm(state), state->common->string.iter, klapi_access(state, -1));
  if (kl_unlikely(exception))
    return klapi_throw_internal(state, KL_E_OOM, "out of memory while setting shared field");
  return klapi_return(state, 0);
}


static void kllib_basic_print_inner(KlValue* val) {
  switch (klvalue_gettype(val)) {
    case KL_INT: {
      fprintf(stdout, "%lld", klvalue_getint(val));
      break;
    }
    case KL_FLOAT: {
      fprintf(stdout, "%lf", klvalue_getfloat(val));
      break;
    }
    case KL_STRING: {
      fputs("\"", stdout);
      fputs(klstring_content(klvalue_getobj(val, KlString*)), stdout);
      fputs("\"", stdout);
      break;
    }
    case KL_NIL: {
      fputs("nil", stdout);
      break;
    }
    case KL_BOOL: {
      fputs(klvalue_getbool(val) ? "true" : "false", stdout);
      break;
    }
    case KL_CFUNCTION: {
      fprintf(stdout, "<%s: %p>", klvalue_typename(klvalue_gettype(val)), klvalue_getgcobj(val));
      break;
    }
    case KL_ARRAY: {
      fputs("[...]", stdout);
      break;
    }
    case KL_MAP: {
      fputs("{:...:}", stdout);
      break;
    }
    default: {
      fprintf(stdout, "<%s: %p>", klvalue_typename(klvalue_gettype(val)), klvalue_getgcobj(val));
      break;
    }
  }
}

static void kllib_basic_print_array(KlArray* array) {
  KlArrayIter end = klarray_iter_end(array);
  KlArrayIter itr = klarray_iter_begin(array);
  if (itr == end) {
    fputs("[]", stdout);
    return;
  }
  fputc('[', stdout);
  kllib_basic_print_inner(itr);
  itr = klarray_iter_next(itr);
  for (; itr != end; itr = klarray_iter_next(itr)) {
    fputs(", ", stdout);
    kllib_basic_print_inner(itr);
  }
  fputc(']', stdout);
}

static void kllib_basic_print_map(KlMap* map) {
  KlMapIter end = klmap_iter_end(map);
  KlMapIter itr = klmap_iter_begin(map);
  if (itr == end) {
    fputs("{::}", stdout);
    return;
  }
  fputs("{:", stdout);
  kllib_basic_print_inner(&itr->key);
  fputc(':', stdout);
  kllib_basic_print_inner(&itr->value);
  itr = klmap_iter_next(itr);
  for (; itr != end; itr = klmap_iter_next(itr)) {
    fputs(", ", stdout);
    kllib_basic_print_inner(&itr->key);
    fputc(':', stdout);
    kllib_basic_print_inner(&itr->value);
  }
  fputs(":}", stdout);
}

static KlException kllib_basic_print(KlState* state) {
  size_t narg = klapi_narg(state);
  for (size_t i = 0; i < narg; ++i) {
    switch (klapi_gettypeb(state, i)) {
      case KL_INT: {
        fprintf(stdout, "%lld", klapi_getintb(state, i));
        break;
      }
      case KL_FLOAT: {
        fprintf(stdout, "%lf", klapi_getfloatb(state, i));
        break;
      }
      case KL_STRING: {
        fputs(klstring_content(klapi_getstringb(state, i)), stdout);
        break;
      }
      case KL_ARRAY: {
        kllib_basic_print_array(klapi_getarrayb(state, i));
        break;
      }
      case KL_MAP: {
        kllib_basic_print_map(klapi_getmapb(state, i));
        break;
      }
      case KL_NIL: {
        fputs("nil", stdout);
        break;
      }
      case KL_BOOL: {
        fputs(klapi_getboolb(state, i) ? "true" : "false", stdout);
        break;
      }
      case KL_CFUNCTION: {
        fprintf(stdout, "<%s: %p>", klvalue_typename(klapi_gettypeb(state, i)), klapi_getcfuncb(state, i));
        break;
      }
      default: {
        fprintf(stdout, "<%s: %p>", klvalue_typename(klapi_gettypeb(state, i)), klapi_getgcobjb(state, i));
        break;
      }
    }
  }
  return KL_E_NONE;
}

static KlException kllib_basic_map_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 3 arguments(1 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 4));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_MAP) && !(klvalue_checktype(base, KL_OBJECT) && klmap_compatiable(klvalue_getobj(base, KlObject*)))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlMap* map = klvalue_getobj(base, KlMap*);
  KlUInt bucketid = klvalue_getuint(base + 1);
  if (kl_unlikely(!klmap_validbucket(map, bucketid)))
    return klapi_throw_internal(state, KL_E_INVLD, "the for loop is broken");
  KlMapIter itr = klmap_getbucket(map, bucketid);
  itr = klmap_bucketnext(map, bucketid, itr);
  if (kl_unlikely(!itr)) return klapi_return(state, 0);
  klvalue_setuint(base + 1, klmap_bucketid(map, itr));
  klvalue_setvalue(base + 2, &itr->key);
  klvalue_setvalue(base + 3, &itr->value);
  return klapi_return(state, 4);
}

static KlException kllib_basic_arr_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 2))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 2 arguments(0 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 3));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_ARRAY) && !(klvalue_checktype(base, KL_OBJECT) && klarray_compatiable(klvalue_getobj(base, KlObject*)))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected array");
  KlArray* array = klvalue_getobj(base, KlArray*);
  size_t index = klvalue_getuint(base + 1);
  if (kl_unlikely(index >= klarray_size(array)))
    return klapi_return(state, 0);
  klvalue_setuint(base + 1, index + 1);
  klvalue_setvalue(base + 2, klarray_access(array, index + 1));
  return klapi_return(state, 3);
}

static KlException kllib_basic_map_iter(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -1, KL_MAP) && !(klapi_checktype(state, -1, KL_OBJECT) && klmap_compatiable(klapi_getobj(state, -1, KlObject*))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlMap* map = klapi_getmap(state, -1);
  if (klmap_size(map) == 0) return klapi_return(state, 0);
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  KlMapIter itr = klmap_iter_begin(map);
  klapi_pushobj(state, map, KL_MAP);
  klapi_pushuint(state, klmap_bucketid(map, itr));
  klapi_pushvalue(state, &itr->key);
  klapi_pushvalue(state, &itr->value);
  klapi_setcfunc(state, -5, kllib_basic_map_next);
  kl_assert(klapi_framesize(state) == 5, "");
  return klapi_return(state, 5);
}

static KlException kllib_basic_arr_iter(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -1, KL_ARRAY) && !(klapi_checktype(state, -1, KL_OBJECT) && klarray_compatiable(klapi_getobj(state, -1, KlObject*))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected array");
  KlArray* array = klapi_getarray(state, -1);
  if (klarray_size(array) == 0) return klapi_return(state, 0);
  KLAPI_PROTECT(klapi_checkstack(state, 3));
  klapi_pushobj(state, array, KL_ARRAY);
  klapi_pushuint(state, 0);
  klapi_pushvalue(state, klarray_access(array, 0));
  klapi_setcfunc(state, -4, kllib_basic_arr_next);
  kl_assert(klapi_framesize(state) == 4, "");
  return klapi_return(state, 4);
}

