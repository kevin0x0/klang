#ifndef _KLANG_INCLUDE_VALUE_KLVALUE_H_
#define _KLANG_INCLUDE_VALUE_KLVALUE_H_

#include "include/value/klcfunc.h"
#include "include/lang/kltypes.h"
#include "include/mm/klmm.h"

#include <stdbool.h>

#define klvalue_gettype(value)              ((value)->type)
#define klvalue_collectable(value)          (klvalue_gettype((value)) >= KL_COLLECTABLE)
#define klvalue_callable(value)             (klvalue_gettype((value)) >= KL_CALLABLEOBJ || klvalue_gettype((value)) == KL_CFUNCTION)
#define klvalue_canrawequal(value)          (klvalue_gettype((value)) <= KL_RAWEQUAL)
#define klvalue_canequal(value)             (klvalue_gettype((value)) <= KL_EQUAL)
#define klvalue_checktype(value, valtype)   (klvalue_gettype((value)) == (valtype))
#define klvalue_isnumber(value)             (klvalue_gettype((value)) < KL_NUMBER)
#define klvalue_isstring(value)             (klvalue_checktype((value), KL_STRING) || klvalue_checktype((value), KL_LSTRING))
#define klvalue_isstrornumber(value)        (klvalue_isstring((value)) || klvalue_isnumber((value)))
#define klvalue_sametype(val1, val2)        (klvalue_gettype(val1) == klvalue_gettype(val2))

#define klvalue_testtag(value, tag)         (klvalue_gettag((value)) & (tag))

#define klvalue_getobj(val, type)           ((type)((val)->value.gcobj))
#define klvalue_setobj(val, obj, type)      klvalue_setgcobj((val), (KlGCObject*)(obj), (type))
#define klvalue_setstring(val, str)         klvalue_setgcobj((val), (KlGCObject*)(str), klvalue_getstringtype((str)))

#define klvalue_equal(v1, v2)               (klvalue_sametype((v1), (v2)) && klvalue_sameinstance((v1), (v2)))

#define klvalue_bothinteger(v1, v2)         (klvalue_gettype(v1) + klvalue_gettype(v2) == KL_INT)
#define klvalue_bothnumber(v1, v2)          (klvalue_gettype(v1) + klvalue_gettype(v2) <= KL_NUMBER)

#define klvalue_getstringtype(str)          (klstring_islong((str)) ? KL_LSTRING : KL_STRING)

#define KLVALUE_NIL_INITWITHTAG(tagval)     { .typewithtag = { .type = KL_NIL, .tag = (tagval) }, .value.nilval = 0 }
#define KLVALUE_NIL_INIT                    KLVALUE_NIL_INITWITHTAG(0)

#define klvalue_nil()                       ((KlValue) { .type = KL_NIL, .value.nilval = 0 })
#define klvalue_int(val)                    ((KlValue) { .type = KL_INT, .value.intval = (val) })
#define klvalue_float(val)                  ((KlValue) { .type = KL_FLOAT, .value.floatval = (val) })
#define klvalue_bool(val)                   ((KlValue) { .type = KL_BOOL, .value.boolval = (val) })
#define klvalue_cfunc(val)                  ((KlValue) { .type = KL_CFUNCTION, .value.cfunc = (val) })
#define klvalue_obj(val, typetag)           ((KlValue) { .type = (typetag), .value.gcobj = klmm_to_gcobj((val)) })
#define klvalue_string(val)                 klvalue_obj((val), klvalue_getstringtype((val)))


typedef enum tagKlType {
  KL_INT = 0, KL_FLOAT,
  KL_NUMBER,  /* not actual type, KL_NUMBER is used for number(KlInt or KlFloat) fast test */
  KL_NIL, KL_BOOL, KL_CFUNCTION,
  KL_USERDATA,                    /* non-collectable type */
  KL_COLLECTABLE , KL_STRING = KL_COLLECTABLE,
  KL_RAWEQUAL = KL_STRING,
  KL_LSTRING, /* long string */
  KL_EQUAL = KL_LSTRING,
  KL_MAP, KL_ARRAY, KL_TUPLE, KL_OBJECT,
  KL_CLASS, KL_KFUNCTION,
  KL_CALLABLEOBJ, KL_KCLOSURE = KL_CALLABLEOBJ, KL_CCLOSURE, KL_COROUTINE,
  KL_CALLABLEOBJ_END = KL_COROUTINE,
  KL_NTYPE, /* number of types */
} KlType;

typedef KlLangInt KlInt;
typedef KlLangUInt KlUInt;
typedef KlLangFloat KlFloat;
typedef KlLangBool KlBool;

#define KL_TRUE   KLLANG_TRUE
#define KL_FALSE  KLLANG_FALSE


typedef struct tagKlValue {
  union {
    KlInt nilval;
    KlInt intval;
    KlFloat floatval;
    KlBool boolval;
    KlCFunction* cfunc;
    KlGCObject* gcobj;
    void* ud;
    KlUInt uintval;
  } value;
  union {
    struct {
      KlType type;
    };
    struct {
      KlType type;
      KlUnsigned tag;
    } typewithtag;
  };
} KlValue;

const char* klvalue_typename(KlType type);


static inline KlInt klvalue_getnil(const KlValue* val) {
  return val->value.nilval;
}

static inline KlInt klvalue_getint(const KlValue* val) {
  return val->value.intval;
}

static inline KlUInt klvalue_getuint(const KlValue* val) {
  return val->value.uintval;
}

static inline KlFloat klvalue_getfloat(const KlValue* val) {
  return val->value.floatval;
}

static inline KlFloat klvalue_getnumber(const KlValue* val) {
  return klvalue_checktype(val, KL_INT) ? klcast(KlFloat, val->value.intval) : val->value.floatval;
}

static inline KlBool klvalue_getbool(const KlValue* val) {
  return val->value.boolval;
}

static inline KlCFunction* klvalue_getcfunc(const KlValue* val) {
  return val->value.cfunc;
}

static inline KlGCObject* klvalue_getgcobj(const KlValue* val) {
  return val->value.gcobj;
}

static inline void* klvalue_getuserdata(const KlValue* val) {
  return val->value.ud;
}

static inline void klvalue_setint(KlValue *val, KlInt intval) {
  val->value.intval = intval;
  val->type = KL_INT;
}

static inline void klvalue_setint_withtag(KlValue *val, KlInt intval, KlUnsigned tag) {
  val->value.intval = intval;
  val->typewithtag.type = KL_INT;
  val->typewithtag.tag = tag;
}

static inline void klvalue_setfloat(KlValue* val, KlFloat floatval) {
  val->value.floatval = floatval;
  val->type = KL_FLOAT;
}

static inline void klvalue_setbool(KlValue *val, KlBool boolval) {
  val->value.boolval = boolval;
  val->type = KL_BOOL;
}

static inline void klvalue_setcfunc(KlValue *val, KlCFunction* cfunc) {
  val->value.cfunc = cfunc;
  val->type = KL_CFUNCTION;
}

static inline void klvalue_setgcobj(KlValue* val, KlGCObject* gcobj, KlType type) {
  val->value.gcobj = gcobj;
  val->type = type; 
}

static inline void klvalue_setuserdata(KlValue* val, void* ud) {
  val->value.ud = ud;
  val->type = KL_USERDATA; 
}

static inline void klvalue_setnil(KlValue *val) {
  val->value.nilval = 0;
  val->type = KL_NIL;
}

static inline void klvalue_setvalue(KlValue *val, const KlValue *other) {
  *val = *other;
}

static inline void klvalue_setvalue_withtag(KlValue *val, const KlValue *other, KlUnsigned tag) {
  val->value = other->value;
  val->typewithtag.type = other->typewithtag.type;
  val->typewithtag.tag = tag;
}

static inline KlUnsigned klvalue_gettag(const KlValue* val) {
  return val->typewithtag.tag;
}

static inline void klvalue_settag(KlValue* val, KlUnsigned tag) {
  val->typewithtag.tag = tag;
}


static inline bool klvalue_sameinstance(const KlValue* val1, const KlValue* val2) {
  kl_assert(klvalue_sametype(val1, val2), "must call this function with two values with same type");
  switch (klvalue_gettype(val1)) {
    case KL_INT: {
      return klvalue_getint(val1) == klvalue_getint(val2);
    }
    case KL_FLOAT: {
      return klvalue_getfloat(val1) == klvalue_getfloat(val2);
    }
    case KL_BOOL: {
      return klvalue_getbool(val1) == klvalue_getbool(val2);
    }
    case KL_NIL: {
      return klvalue_getnil(val1) == klvalue_getnil(val2);
    }
    case KL_CFUNCTION: {
      return klvalue_getcfunc(val1) == klvalue_getcfunc(val2);
    }
    default: {
      return klvalue_getgcobj(val1) == klvalue_getgcobj(val2);
    }
  }
}

#endif
