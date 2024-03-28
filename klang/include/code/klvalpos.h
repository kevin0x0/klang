#ifndef KEVCC_KLANG_INCLUDE_CODE_KLVALKIND_H
#define KEVCC_KLANG_INCLUDE_CODE_KLVALKIND_H

typedef enum tagKlValKind {
  KLVAL_STACK,
  KLVAL_REF,
  KLVAL_BOOL,
  KLVAL_NIL,
  KLVAL_STRING,
  KLVAL_INTEGER,  
  KLVAL_FLOAT,  
} KlValKind;

#define klvalkind_isnumber(v)   ((v).kind == KLVAL_FLOAT || (v).kind == KLVAL_INTEGER)

#endif

