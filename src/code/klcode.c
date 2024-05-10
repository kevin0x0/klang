#include "include/code/klcode.h"
#include "include/ast/klstrtbl.h"
#include "include/code/klgen.h"
#include "include/code/klsymtbl.h"
#include "include/lang/kltypes.h"
#include "include/misc/klutils.h"



KlCode* klcode_create(KlCRefInfo* refinfo, unsigned nref, KlConstant* constants, unsigned nconst,
                      KlInstruction* code, KlFilePosition* posinfo, unsigned codelen,
                      KlCode** nestedfunc, unsigned nnested, KlStrTbl* strtbl, unsigned nparam,
                      unsigned framesize, KlStrDesc srcfile) {
  KlCode* klcode = (KlCode*)malloc(sizeof (KlCode));
  if (kl_unlikely(!klcode)) return NULL;
  klcode->refinfo = refinfo;
  klcode->nref = nref;
  klcode->constants = constants;
  klcode->nconst = nconst;
  klcode->code = code;
  klcode->posinfo = posinfo;
  klcode->srcfile = srcfile;
  klcode->codelen = codelen;
  klcode->nestedfunc = nestedfunc;
  klcode->nnested = nnested;
  klcode->strtbl = strtbl;
  klcode->nparam = nparam;
  klcode->framesize = framesize;
  return klcode;
}

void klcode_delete(KlCode* code) {
  free(code->refinfo);
  free(code->constants);
  free(code->code);
  free(code->posinfo);
  size_t nnested = code->nnested;
  KlCode** codes = code->nestedfunc;
  for (size_t i = 0; i < nnested; ++i) {
    klcode_delete(codes[i]);
  }
  free(codes);
  free(code);
}

KlCode* klcode_create_fromast(KlAstStmtList* ast, KlStrTbl* strtbl, KlCodeGenConfig* config) {
  return klgen_file(ast, strtbl, config);
}
