#include "include/code/klcode.h"
#include "include/code/klgen.h"
#include "include/code/klsymtbl.h"
#include "include/misc/klutils.h"



KlCode* klcode_create(KlRefInfo* refinfo, size_t nref, KlConstant* constants, size_t nconst,
                      KlInstruction* code, KlFilePosition* lineinfo, size_t codelen,
                      KlCode** nestedfunc, size_t nnested, KlStrTbl* strtbl, size_t nparam,
                      size_t framesize) {
  KlCode* klcode = (KlCode*)malloc(sizeof (KlCode));
  if (kl_unlikely(!klcode)) return NULL;
  klcode->refinfo = refinfo;
  klcode->nref = nref;
  klcode->constants = constants;
  klcode->nconst = nconst;
  klcode->code = code;
  klcode->posinfo = lineinfo;
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

KlCode* klcode_create_fromcst(KlCst* cst, KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, bool debug) {
  return klgen_file(cst, strtbl, input, inputname, klerr, debug);
}
