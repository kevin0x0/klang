#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODEVAL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODEVAL_H

#include "klang/include/cst/klstrtbl.h"
#include "klang/include/value/klvalue.h"
#include <stddef.h>

typedef enum tagKlValKind {
  KLVAL_BOOL,
  KLVAL_NIL,
  KLVAL_STRING,
  KLVAL_INTEGER,  
  KLVAL_FLOAT,  
  KLVAL_JMP,
  KLVAL_STACK,
  KLVAL_REF,
  KLVAL_NONE,   /* currently only used for boolean expression */
} KlValKind;

typedef struct tagKlCodeVal {
  KlValKind kind;
  union {
    size_t index;
    struct {
      ptrdiff_t tail;
      ptrdiff_t head;
    } jmplist;
    KlInt intval;
    KlFloat floatval;
    KlBool boolval;
    KlStrDesc string;
  };
} KlCodeVal;

#define klcodeval_isnumber(v)   ((v).kind == KLVAL_FLOAT || (v).kind == KLVAL_INTEGER)
#define klcodeval_isconstant(v) ((v).kind <= KLVAL_FLOAT)
#define klcodeval_isfalse(v)    (((v).kind == KLVAL_BOOL && (v).boolval == KL_FALSE) || (v).kind == KLVAL_NIL)
#define klcodeval_istrue(v)     (!klcodeval_isfalse((v)))

static inline KlCodeVal klcodeval_index(KlValKind kind, size_t index);
static inline KlCodeVal klcodeval_stack(size_t index);
static inline KlCodeVal klcodeval_ref(size_t index);
static inline KlCodeVal klcodeval_string(KlStrDesc str);
static inline KlCodeVal klcodeval_integer(KlInt intval);
static inline KlCodeVal klcodeval_float(KlInt floatval);
static inline KlCodeVal klcodeval_bool(KlBool boolval);
static inline KlCodeVal klcodeval_nil(void);
static inline KlCodeVal klcodeval_jmp(size_t pc);
static inline KlCodeVal klcodeval_none(void);

static inline KlCodeVal klcodeval_index(KlValKind kind, size_t index) {
  KlCodeVal val = { .kind = kind, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_stack(size_t index) {
  KlCodeVal val = { .kind = KLVAL_STACK, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_ref(size_t index) {
  KlCodeVal val = { .kind = KLVAL_REF, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_string(KlStrDesc str) {
  KlCodeVal val = { .kind = KLVAL_STRING, .string = str };
  return val;
}

static inline KlCodeVal klcodeval_integer(KlInt intval) {
  KlCodeVal val = { .kind = KLVAL_INTEGER, .intval = intval };
  return val;
}

static inline KlCodeVal klcodeval_float(KlInt floatval) {
  KlCodeVal val = { .kind = KLVAL_FLOAT, .intval = floatval };
  return val;
}

static inline KlCodeVal klcodeval_bool(KlBool boolval) {
  KlCodeVal val = { .kind = KLVAL_BOOL, .boolval = boolval };
  return val;
}

static inline KlCodeVal klcodeval_nil(void) {
  KlCodeVal val = { .kind = KLVAL_NIL };
  return val;
}

static inline KlCodeVal klcodeval_jmp(size_t pc) {
  KlCodeVal val = { .kind = KLVAL_JMP, .jmplist = { .head = pc, .tail = pc } };
  return val;
}

static inline KlCodeVal klcodeval_none(void) {
  KlCodeVal val = { .kind = KLVAL_NONE };
  return val;
}

#endif

