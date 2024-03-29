#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODEVAL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODEVAL_H

#include "klang/include/cst/klstrtab.h"
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
} KlValKind;

/* here begins the code generation */
typedef struct tagKlCodeVal {
  KlValKind kind;
  union {
    size_t index;
    size_t pc;
    KlInt intval;
    KlFloat floatval;
    KlBool boolval;
    KlStrDesc string;
  };
} KlCodeVal;

#define klcodeval_isnumber(v)   ((v).kind == KLVAL_FLOAT || (v).kind == KLVAL_INTEGER)
#define klcodeval_isconstant(v) ((v).kind <= KLVAL_FLOAT)

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

static inline KlCodeVal klcodeval_bool(KlBool intval) {
  KlCodeVal val = { .kind = KLVAL_BOOL, .intval = intval };
  return val;
}

static inline KlCodeVal klcodeval_nil(void) {
  KlCodeVal val = { .kind = KLVAL_NIL };
  return val;
}

static inline KlCodeVal klcodeval_jmp(size_t pc) {
  KlCodeVal val = { .kind = KLVAL_JMP, .pc = pc };
  return val;
}

#endif

