#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODE_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODE_H

#include "include/code/klcontbl.h"
#include "include/code/klsymtbl.h"
#include "include/value/klref.h"
#include "include/vm/klinst.h"
#include "deps/k/include/array/kgarray.h"


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
  KlFilePosition* posinfo;
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

KlCode* klcode_create_fromcst(KlCst* cst, KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, bool debug);
void klcode_print(KlCode* code, Ko* out);

#endif
