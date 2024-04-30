#ifndef KEVCC_KLANG_INCLUDE_CODE_KLCODE_H
#define KEVCC_KLANG_INCLUDE_CODE_KLCODE_H

#include "include/code/klcontbl.h"
#include "include/lang/klinst.h"


typedef struct tagKlCode KlCode;
typedef struct tagKlFilePosition {
  unsigned begin;
  unsigned end;
} KlFilePosition;

typedef struct tagKlCodeGenConfig {
  const char* inputname;
  bool debug;
  bool posinfo;
  KlError* klerr;
  Ki* input;
} KlCodeGenConfig;

/* this is the same structure as KlRefInfo used in compiler */
typedef struct tagKlCRefInfo {
  unsigned index;
  bool on_stack;
} KlCRefInfo;

struct tagKlCode {
  KlCRefInfo* refinfo;
  KlConstant* constants;
  KlInstruction* code;
  KlFilePosition* posinfo;
  KlCode** nestedfunc;        /* functions created inside this function */
  KlStrTbl* strtbl;
  unsigned nref;
  unsigned nconst;
  unsigned codelen;
  unsigned nnested;
  unsigned nparam;            /* number of parameters */
  unsigned framesize;         /* stack frame size of this klang function */
};


KlCode* klcode_create(KlCRefInfo* refinfo, size_t nref, KlConstant* constants, size_t nconst,
                      KlInstruction* code, KlFilePosition* posinfo, size_t codelen,
                      KlCode** nestedfunc, size_t nnested, KlStrTbl* strtbl, size_t nparam,
                      size_t framesize);
void klcode_delete(KlCode* code);

KlCode* klcode_create_fromcst(KlCst* cst, KlStrTbl* strtbl, KlCodeGenConfig* config);
KlCode* klcode_create_fromfile(Ki* file);

bool klcode_dump(KlCode* code, Ko* file);
void klcode_print(KlCode* code, Ko* out);

#endif
