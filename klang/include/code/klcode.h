#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODE_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODE_H

#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/value/klref.h"
#include "klang/include/vm/klinst.h"
#include "utils/include/array/kgarray.h"


typedef struct tagKlCode KlCode;
typedef struct tagKlFilePosition {
  KlFileOffset begin;
  KlFileOffset end;
} KlFilePosition;


struct tagKlCode {
  KlRefInfo* refinfo;
  size_t nref;
  KlConstant* constants;
  size_t nconst;
  KlInstruction* code;
  size_t codelen;
  KlFilePosition* lineinfo;
  KlCode** nestedfunc;        /* functions created inside this function */
  size_t nnested;
  KlStrTbl* strtbl;
  size_t nparam;              /* number of parameters */
  size_t framesize;           /* stack frame size of this klang function */
};


KlCode* klcode_create(KlRefInfo* refinfo, size_t nref, KlConstant* constants, size_t nconst,
                      KlInstruction* code, KlFilePosition* lineinfo, size_t codelen,
                      KlCode** nestedfunc, size_t nnested, KlStrTbl* strtbl, size_t nparam,
                      size_t framesize);
void klcode_delete(KlCode* code);

#endif
