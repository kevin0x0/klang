#include "include/klapi.h"
#include "include/value/klbuiltinclass.h"
#include "include/value/klarray.h"
#include "include/value/klcfunc.h"
#include "include/value/klmap.h"
#include "include/value/klstate.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"
#include <stdio.h>

#define KLLIB_BASIC_PRINT_DEPTH_LIMIT   (3)

static void kllib_basic_print_map(KlState* state, const KlMap* map, size_t depth);
static void kllib_basic_print_inner(KlState* state, const KlValue* val, size_t depth);
static void kllib_basic_print_array(KlState* state, const KlArray* array, size_t depth);
static KlException kllib_basic_print(KlState* state);
static KlException kllib_basic_map_next(KlState* state);
static KlException kllib_basic_arr_next(KlState* state);
static KlException kllib_basic_callable_next(KlState* state);
static KlException kllib_basic_map_iter(KlState* state);
static KlException kllib_basic_arr_iter(KlState* state);
static KlException kllib_basic_callable_iter(KlState* state);
static KlException kllib_basic_map_weak(KlState* state);

static KlException kllib_basic_init_globalvar(KlState* state);
static KlException kllib_basic_init_iter(KlState* state);
static KlException kllib_basic_init_map(KlState* state);

KlException kllib_init(KlState* state) {
  KLAPI_PROTECT(kllib_basic_init_globalvar(state));
  KLAPI_PROTECT(kllib_basic_init_iter(state));
  KLAPI_PROTECT(kllib_basic_init_map(state));
  return klapi_return(state, 0);
}

static KlException kllib_basic_init_globalvar(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KLAPI_PROTECT(klapi_pushstring(state, "print"));
  klapi_pushcfunc(state, kllib_basic_print);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "string"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_STRING], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "integer"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_INT], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "float"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_FLOAT], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "coroutine"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_COROUTINE], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "boolean"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_BOOL], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "closure"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_KCLOSURE], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "map"));
  klapi_setobj(state, -1, state->common->klclass.map, KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "array"));
  klapi_setobj(state, -1, state->common->klclass.array, KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  klapi_pop(state, 2);
  return KL_E_NONE;
}

static KlException kllib_basic_init_iter(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushcfunc(state, kllib_basic_map_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.map, state->common->string.iter));
  klapi_setcfunc(state, -1, kllib_basic_arr_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.array, state->common->string.iter));
  klapi_setcfunc(state, -1, kllib_basic_callable_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_COROUTINE], state->common->string.iter));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

static KlException kllib_basic_init_map(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KLAPI_PROTECT(klapi_pushstring(state, "weak"));
  klapi_pushcfunc(state, kllib_basic_map_weak);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.map, klapi_getstring(state, -2)));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

static void kllib_basic_print_inner(KlState* state, const KlValue* val, size_t depth) {
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
      fprintf(stdout, "<%s: %p>", klexec_typename(state, val), klvalue_getgcobj(val));
      break;
    }
    case KL_ARRAY: {
      kllib_basic_print_array(state, klvalue_getobj(val, KlArray*), depth + 1);
      break;
    }
    case KL_MAP: {
      kllib_basic_print_map(state, klvalue_getobj(val, KlMap*), depth + 1);
      break;
    }
    default: {
      fprintf(stdout, "<%s: %p>", klexec_typename(state, val), klvalue_getgcobj(val));
      break;
    }
  }
}

static void kllib_basic_print_array(KlState* state, const KlArray* array, size_t depth) {
  KlArrayIter end = klarray_iter_end(array);
  KlArrayIter itr = klarray_iter_begin(array);
  if (itr == end) {
    fputs("[]", stdout);
    return;
  }
  if (depth++ >= KLLIB_BASIC_PRINT_DEPTH_LIMIT) {
    fputs("[...]", stdout);
    return;
  }
  fputc('[', stdout);
  kllib_basic_print_inner(state, itr, depth);
  itr = klarray_iter_next(itr);
  for (; itr != end; itr = klarray_iter_next(itr)) {
    fputs(", ", stdout);
    kllib_basic_print_inner(state, itr, depth);
  }
  fputc(']', stdout);
}

static void kllib_basic_print_map(KlState* state, const KlMap* map, size_t depth) {
  size_t end = klmap_iter_end(map);
  size_t itr = klmap_iter_begin(map);
  if (itr == end) {
    fputs("{:}", stdout);
    return;
  }
  if (depth++ >= KLLIB_BASIC_PRINT_DEPTH_LIMIT) {
    fputs("{...}", stdout);
    return;
  }
  fputc('{', stdout);
  kllib_basic_print_inner(state, klmap_iter_getkey(map, itr), depth);
  fputc(':', stdout);
  kllib_basic_print_inner(state, klmap_iter_getvalue(map, itr), depth);
  itr = klmap_iter_next(map, itr);
  for (; itr != end; itr = klmap_iter_next(map, itr)) {
    fputs(", ", stdout);
    kllib_basic_print_inner(state, klmap_iter_getkey(map, itr), depth);
    fputc(':', stdout);
    kllib_basic_print_inner(state, klmap_iter_getvalue(map, itr), depth);
  }
  fputc('}', stdout);
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
        kllib_basic_print_array(state, klapi_getarrayb(state, i), 0);
        break;
      }
      case KL_MAP: {
        kllib_basic_print_map(state, klapi_getmapb(state, i), 0);
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
        fprintf(stdout, "<%s: %p>", klexec_typename(state, klapi_accessb(state, i)), klapi_getcfuncb(state, i));
        break;
      }
      default: {
        fprintf(stdout, "<%s: %p>", klexec_typename(state, klapi_accessb(state, i)), klapi_getgcobjb(state, i));
        break;
      }
    }
    fputc('\t', stdout);
  }
  fputc('\n', stdout);
  return klapi_return(state, 0);
}

static KlException kllib_basic_map_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 3 arguments(1 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 4));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_MAP) && !(klvalue_checktype(base, KL_OBJECT) && klmap_compatible(klvalue_getobj(base, KlObject*)))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlMap* map = klvalue_getobj(base, KlMap*);
  size_t index = klvalue_getint(base + 1);
  if (kl_unlikely(klmap_iter_valid(map, index)))
    return klapi_throw_internal(state, KL_E_INVLD, "the for loop is broken");
  index = klmap_iter_next(map, index);
  if (kl_unlikely(index == klmap_iter_end(map)))
    return klapi_return(state, 0);
  klvalue_setint(base + 1, index);
  klvalue_setvalue(base + 2, klmap_iter_getkey(map, index));
  klvalue_setvalue(base + 3, klmap_iter_getvalue(map, index));
  return klapi_return(state, 4);
}

static KlException kllib_basic_arr_next_with_index(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 2))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 2 arguments(0 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 4));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_ARRAY) && !(klvalue_checktype(base, KL_OBJECT) && klarray_compatible(klvalue_getobj(base, KlObject*)))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected array");
  KlArray* array = klvalue_getobj(base, KlArray*);
  KlUInt index = klvalue_getint(base + 1) + 1;
  if (kl_unlikely(index >= klarray_size(array)))
    return klapi_return(state, 0);
  klvalue_setint(base + 1, klcast(KlInt, index));
  klvalue_setint(base + 2, klcast(KlInt, index));
  klvalue_setvalue(base + 3, klarray_access(array, index));
  return klapi_return(state, 4);
}

static KlException kllib_basic_arr_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 2))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 2 arguments(0 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 3));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_ARRAY) && !(klvalue_checktype(base, KL_OBJECT) && klarray_compatible(klvalue_getobj(base, KlObject*)))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected array");
  KlArray* array = klvalue_getobj(base, KlArray*);
  KlUInt index = klvalue_getint(base + 1) + 1;
  if (kl_unlikely(index >= klarray_size(array)))
    return klapi_return(state, 0);
  klvalue_setint(base + 1, index);
  klvalue_setvalue(base + 2, klarray_access(array, index));
  klapi_setframesize(state, 3);
  return klapi_return(state, 3);
}

static KlException kllib_basic_callable_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 2))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 2 arguments(0 iteration variable in for loop)");
  if (klapi_nres(state) < 3)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected at least 3 returned values");
  if (klapi_nres(state) == KLAPI_VARIABLE_RESULTS)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected fixed number of returned values");
  size_t nval = klapi_nres(state) - 2;
  klapi_setframesize(state, 2);
  KLAPI_PROTECT(klapi_scall(state, klapi_accessb(state, 0), 0, nval));
  if (klapi_checktype(state, -nval, KL_NIL))
    return klapi_return(state, 0);
  return klapi_return(state, klapi_nres(state));
}

static KlException kllib_basic_map_iter(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -1, KL_MAP) && !(klapi_checktype(state, -1, KL_OBJECT) && klmap_compatible(klapi_getobj(state, -1, KlObject*))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlMap* map = klapi_getmap(state, -1);
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  size_t index = klmap_iter_begin(map);
  if (index == klmap_iter_end(map))
    return klapi_return(state, 0);
  klapi_pushobj(state, map, KL_MAP);
  klapi_pushint(state, index);
  klapi_pushvalue(state, klmap_iter_getkey(map, index));
  klapi_pushvalue(state, klmap_iter_getvalue(map, index));
  klapi_setcfunc(state, -5, kllib_basic_map_next);
  kl_assert(klapi_framesize(state) == 5, "");
  return klapi_return(state, 5);
}

static KlException kllib_basic_arr_iter(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -1, KL_ARRAY) && !(klapi_checktype(state, -1, KL_OBJECT) && klarray_compatible(klapi_getobj(state, -1, KlObject*))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected array");
  KlArray* array = klapi_getarray(state, -1);
  if (klarray_size(array) == 0) return klapi_return(state, 0);
  KLAPI_PROTECT(klapi_checkstack(state, 4));
  klapi_pushobj(state, array, KL_ARRAY);
  klapi_pushint(state, 0);
  if (klapi_nres(state) == 4) {
    klapi_pushvalue(state, klarray_access(array, 0));
    klapi_setcfunc(state, -4, kllib_basic_arr_next);
    kl_assert(klapi_framesize(state) == 4, "");
    return klapi_return(state, 4);
  } else {
    klapi_pushint(state, 0);
    klapi_pushvalue(state, klarray_access(array, 0));
    klapi_setcfunc(state, -5, kllib_basic_arr_next_with_index);
    kl_assert(klapi_framesize(state) == 5, "");
    return klapi_return(state, 5);
  }
}

static KlException kllib_basic_callable_iter(KlState* state) {
  if (klapi_narg(state) != 1)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (klapi_nres(state) < 4)
    return klapi_throw_internal(state, KL_E_INVLD, "expected at least 4 returned value");
  if (klapi_nres(state) == KLAPI_VARIABLE_RESULTS)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected fixed number of returned values");
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  klapi_pushvalue(state, klapi_access(state, -1));
  klapi_pushvalue(state, klapi_access(state, -1));
  size_t nval = klapi_nres(state) - 3;
  KLAPI_PROTECT(klapi_scall(state, klapi_access(state, -2), 0, nval));
  if (klapi_checktype(state, -nval, KL_NIL))
    return klapi_return(state, 0);
  klapi_setcfunc(state, -klapi_framesize(state), kllib_basic_callable_next);
  return klapi_return(state, klapi_nres(state));
}

static KlException kllib_basic_map_weak(KlState* state) {
  if (klapi_narg(state) != 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly one argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -2, KL_MAP) && !(klapi_checktype(state, -2, KL_OBJECT) && klmap_compatible(klapi_getobj(state, -2, KlObject*))))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  if (!klapi_checktype(state, -1, KL_STRING))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string(\"k\", \"v\", \"kv\")");
  KlMap* map = klapi_getobj(state, -2, KlMap*);
  KlString* option = klapi_getstring(state, -1);
  if (strcmp(klstring_content(option), "k") == 0) {
    klmap_assignoption(map, KLMAP_OPT_WEAKKEY);
  } else if (strcmp(klstring_content(option), "v") == 0) {
    klmap_assignoption(map, KLMAP_OPT_WEAKVAL);
  } else if (strcmp(klstring_content(option), "kv") == 0) {
    klmap_assignoption(map, KLMAP_OPT_WEAKKEY | KLMAP_OPT_WEAKVAL);
  } else {
    return klapi_throw_internal(state, KL_E_INVLD, "expected string \"k\", \"v\" or \"kv\", got %s", klstring_content(option));
  }
  klapi_pop(state, 1);
  return klapi_return(state, 1);
}
