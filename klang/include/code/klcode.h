#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODE_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODE_H

#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/value/klref.h"
#include "klang/include/vm/klinst.h"
#include "utils/include/array/kgarray.h"


typedef struct tagKlCode KlCode;

struct tagKlCode {
  KlRefInfo* refinfo;
  size_t nref;
  KlConstant* constants;
  size_t nconst;
  KlInstruction* code;
  size_t codelen;
  KlCode** nestedfunc;        /* functions created inside this function */
  size_t nnested;
  KlStrTab* strtab;
  size_t nparam;              /* number of parameters */
  size_t framesize;           /* stack frame size of this klang function */
};


KlCode* klcode_create(KlCst* cst);
void klcode_delete(KlCode* code);

typedef struct tagKlFilePosition {
  KlFileOffset begin;
  KlFileOffset end;
} KlFilePosition;

kgarray_decl(KlCode, KlCodeArray, klcodearr, pass_ref,)
kgarray_decl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_decl(KlFilePosition, KlFPArray, klfparr, pass_val,)

#endif
