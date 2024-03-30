#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPRBOOL_H

#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klgen.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"


KlCodeVal klgen_exprbool(KlGenUnit* gen, KlCst* boolcst, bool jumpcond);
KlCodeVal klgen_exprnot(KlGenUnit* gen, KlCstPre* notcst, bool jumpcond);
KlCodeVal klgen_expror(KlGenUnit* gen, KlCstBin* orcst, bool jumpcond);
KlCodeVal klgen_exprand(KlGenUnit* gen, KlCstBin* andcst, bool jumpcond);

#endif
