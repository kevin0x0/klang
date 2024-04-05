#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H

#include "klang/include/code/klgen.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"


KlCodeVal klgen_exprbool(KlGenUnit* gen, KlCst* cst, bool jumpcond);
KlCodeVal klgen_exprnot(KlGenUnit* gen, KlCstPre* notcst, bool jumpcond);
KlCodeVal klgen_expror(KlGenUnit* gen, KlCstBin* orcst, bool jumpcond);
KlCodeVal klgen_exprand(KlGenUnit* gen, KlCstBin* andcst, bool jumpcond);

KlCodeVal klgen_exprboolval(KlGenUnit* gen, KlCst* cst, size_t target);
void klgen_setinstjmppos(KlGenUnit* gen, KlCodeVal jmplist, size_t jmppos);

#endif
