#include "include/klapi.h"
#include "include/misc/klutils.h"
#include "include/value/klbuiltinclass.h"
#include "include/value/klarray.h"
#include "include/value/klcfunc.h"
#include "include/value/klmap.h"
#include "include/value/klstate.h"
#include "include/value/klvalue.h"
#include "include/vm/klexception.h"

static KlException kllib_basic_map_next(KlState* state);
static KlException kllib_basic_arr_next(KlState* state);
static KlException kllib_basic_callable_next(KlState* state);
static KlException kllib_basic_map_iter(KlState* state);
static KlException kllib_basic_arr_iter(KlState* state);
static KlException kllib_basic_callable_iter(KlState* state);

static KlException kllib_basic_map_weak(KlState* state);
static KlException kllib_basic_map_customhash(KlState* state);
static KlException kllib_basic_map_getslot(KlState* state);
static KlException kllib_basic_map_index(KlState* state);
static KlException kllib_basic_map_indexas(KlState* state);

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
  KLAPI_PROTECT(klapi_pushstring(state, "string"));
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
  klapi_setobj(state, -1, state->common->klclass.phony[KL_MAP], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  KLAPI_PROTECT(klapi_setstring(state, -2, "array"));
  klapi_setobj(state, -1, state->common->klclass.phony[KL_ARRAY], KL_CLASS);
  KLAPI_PROTECT(klapi_storeglobal(state, klapi_getstring(state, -2), -1));
  klapi_pop(state, 2);
  return KL_E_NONE;
}

static KlException kllib_basic_init_iter(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 1));
  klapi_pushcfunc(state, kllib_basic_map_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_MAP], state->common->string.iter));
  klapi_setcfunc(state, -1, kllib_basic_arr_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_ARRAY], state->common->string.iter));
  klapi_setcfunc(state, -1, kllib_basic_callable_iter);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_COROUTINE], state->common->string.iter));
  klapi_pop(state, 1);
  return KL_E_NONE;
}

static KlException kllib_basic_init_map(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 2));
  KLAPI_PROTECT(klapi_pushstring(state, "weak"));
  klapi_pushcfunc(state, kllib_basic_map_weak);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_MAP], klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -1, "customhash"));
  klapi_pushcfunc(state, kllib_basic_map_customhash);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_MAP], klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -1, "=[]"));
  klapi_pushcfunc(state, kllib_basic_map_index);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_MAP], klapi_getstring(state, -2)));

  KLAPI_PROTECT(klapi_setstring(state, -1, "[]="));
  klapi_pushcfunc(state, kllib_basic_map_indexas);
  KLAPI_PROTECT(klapi_class_newshared_method(state, state->common->klclass.phony[KL_MAP], klapi_getstring(state, -2)));

  klapi_pop(state, 1);
  return KL_E_NONE;
}
static KlException kllib_basic_map_next(KlState* state) {
  if (kl_unlikely(klapi_narg(state) < 3))
    return klapi_throw_internal(state, KL_E_ARGNO, "there should be more than 3 arguments(1 iteration variable in for loop)");
  KLAPI_PROTECT(klapi_checkframeandset(state, 4));
  KlValue* base = klapi_accessb(state, 0);
  if (kl_unlikely(!klvalue_checktype(base, KL_MAP)))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlMap* map = klvalue_getobj(base, KlMap*);
  size_t index = klvalue_getint(base + 1);
  if (kl_unlikely(!klmap_iter_valid(map, index)))
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
  if (kl_unlikely(!klvalue_checktype(base, KL_ARRAY)))
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
  if (kl_unlikely(!klvalue_checktype(base, KL_ARRAY)))
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
  if (!klapi_checktype(state, -1, KL_MAP))
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
  if (!klapi_checktype(state, -1, KL_ARRAY))
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
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly two argmument(this method should be automatically called in iterration loop)");
  if (!klapi_checktype(state, -2, KL_MAP))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  if (!klapi_checktype(state, -1, KL_STRING))
    return klapi_throw_internal(state, KL_E_TYPE, "expected string(\"k\", \"v\", \"kv\")");
  KlMap* map = klapi_getmap(state, -2);
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

static KlException kllib_basic_map_customhash(KlState* state) {
  if (klapi_narg(state) == 0)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected at least one argument");
  if (!klapi_checktypeb(state, 0, KL_MAP))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");
  KlBool enable = KL_TRUE;
  if (klapi_narg(state) >= 2)
    enable = klapi_toboolb(state, 1);

  KlMap* map = klapi_getmapb(state, 0);
  enable == KL_TRUE ? klmap_setoption(map, KLMAP_OPT_CUSTOMHASH)
                    : klmap_clroption(map, KLMAP_OPT_CUSTOMHASH);
  klapi_pop(state, klapi_narg(state) - 1);
  return klapi_return(state, 1);
}

/* try get slot, if the slot does not exist, return key's hash */
static KlException kllib_basic_map_getslot(KlState* state) {
  KLAPI_PROTECT(klapi_checkstack(state, 3));

  /* call hash method */
  klapi_pushvalue(state, klapi_access(state, -1));
  KlValue hashmethod;
  bool ismethod = klapi_getmethod(state, klapi_access(state, -1), klstate_common(state)->string.hash, &hashmethod);
  if (kl_unlikely(!ismethod))
    return klapi_throw_internal(state, KL_E_INVLD,
                                "no custom hash method for value with type '%s'",
                                klexec_typename(state, klapi_access(state, -1)));
  KLAPI_PROTECT(klapi_scall(state, &hashmethod, 1, 1));

  /* get hash value */
  if (kl_unlikely(!klapi_checktype(state, -1, KL_INT)))
      return klapi_throw_internal(state, KL_E_TYPE,
                                  "hashmethod should return integer, got '%s'",
                                  klexec_typename(state, klapi_access(state, -1)));
  size_t hash = klcast(size_t, klapi_getint(state, -1));
  klapi_pop(state, 1);
  /* find slot */
  KlMap* map = klapi_getmap(state, -2);
  KlType keytype = klapi_gettype(state, -1);
  KlValue eqmethod;
  ismethod = klapi_getmethod(state, klapi_access(state, -1), klstate_common(state)->string.eq, &eqmethod);
  if (kl_unlikely(!ismethod))
    return klapi_throw_internal(state, KL_E_INVLD,
                                "no equality comparison method for value with type '%s'",
                                klexec_typename(state, klapi_access(state, -1)));
  klapi_pushvalue(state, &eqmethod);  /* push it to avoid being garbage collected */

  KlMapSlot* slot = &map->slots[hash & (klmap_mask(map))];
  if (!klmap_masterslot(slot)) {
    klapi_pushint(state, klcast(KlInt, hash));
    return klapi_return(state, 1);
  }
  do {
    if (hash != slot->hash || !klvalue_checktype(&slot->key, keytype)) {
      slot = slot->next;
      continue;
    }
    /* call '==' method */
    klapi_pushvalue(state, klapi_access(state, -2));
    klapi_pushvalue(state, &slot->key);
    KLAPI_PROTECT(klapi_scall(state, &eqmethod, 2, 1));
    KlBool equal = klapi_tobool(state, -1);
    klapi_pop(state, 1);
    if (equal == KL_TRUE) {
      klapi_pushuserdata(state, slot);
      return klapi_return(state, 1);
    }
    slot = slot->next;
  } while (slot);
  klapi_pushint(state, klcast(KlInt, hash));
  return klapi_return(state, 1);
}

static KlException kllib_basic_map_index(KlState* state) {
  if (klapi_narg(state) != 2)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly two arguments");
  if (!klapi_checktypeb(state, 0, KL_MAP))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");

  KLAPI_PROTECT(klapi_checkstack(state, 2));
  klapi_pushvalue(state, klapi_access(state, -2));
  klapi_pushvalue(state, klapi_access(state, -2));
  KLAPI_PROTECT(klapi_scall(state, &klvalue_cfunc(kllib_basic_map_getslot), 2, 1));
  if (kl_unlikely(klapi_checktype(state, -1, KL_INT)))
    return klapi_return(state, 0);
  KlMapSlot* slot = klcast(KlMapSlot*, klapi_getuserdata(state, -1));
  klapi_pushvalue(state, &slot->value);
  return klapi_return(state, 1);
}

static KlException kllib_basic_map_indexas(KlState* state) {
  if (klapi_narg(state) != 3)
    return klapi_throw_internal(state, KL_E_ARGNO, "expected exactly three arguments");
  if (!klapi_checktypeb(state, 0, KL_MAP))
    return klapi_throw_internal(state, KL_E_TYPE, "expected map");

  KLAPI_PROTECT(klapi_checkstack(state, 2));
  klapi_pushvalue(state, klapi_access(state, -3));
  klapi_pushvalue(state, klapi_access(state, -3));
  KLAPI_PROTECT(klapi_scall(state, &klvalue_cfunc(kllib_basic_map_getslot), 2, 1));
  if (kl_unlikely(klapi_checktype(state, -1, KL_INT))) {
    if (kl_unlikely(!klmap_insert_hash(klapi_getmapb(state, 0),
                                       klstate_getmm(state),
                                       klapi_access(state, -3), /* key */
                                       klapi_access(state, -2), /* value */
                                       klcast(size_t, klapi_getint(state, -1) /* hash value */))))
      return klapi_throw_internal(state, KL_E_OOM, "out of memory while doing indexas");
  } else {
    KlMapSlot* slot = klcast(KlMapSlot*, klapi_getuserdata(state, -1));
    klmap_slot_setvalue(slot, klapi_access(state, -2));
  }
  return klapi_return(state, 0);
}
