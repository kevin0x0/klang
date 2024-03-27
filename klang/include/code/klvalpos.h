#ifndef KEVCC_KLANG_INCLUDE_CODE_KLVALPOS_H
#define KEVCC_KLANG_INCLUDE_CODE_KLVALPOS_H

typedef enum tagKlValKind {
  KLVAL_STACK,
  KLVAL_REF,
  KLVAL_STRING,
  KLVAL_BOOL,
  KLVAL_NIL,
  /* Integer can be contsant(stored in constant table) or immediate value
   * (stored in instruction). This is decided by whether it fits specific
   * range(varies from instruction to instruction). */
  KLVAL_INTEGER,  
} KlValKind;

#endif

