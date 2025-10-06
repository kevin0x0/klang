#ifndef _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_
#define _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_

#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/code/klgen_utils.h"



KlCodeVal klgen_exprconstant(KlGenUnit* gen, KlAstConstant* conast);
KlCodeVal klgen_expr(KlGenUnit* gen, KlAstExpr* ast);
static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlAstExpr* ast);
KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlAstExpr* ast, KlCStkId target);
void klgen_exprlist_raw(KlGenUnit* gen, KlAstExpr** asts, size_t nast, size_t nwanted, KlFilePosition filepos);
size_t klgen_exprwhere_inreturn(KlGenUnit* gen, KlAstWhere* whereast, KlCodeVal* respos);
void klgen_multival(KlGenUnit* gen, KlAstExpr* ast, size_t nval, KlCStkId target);
/* try to generate code for expressions that can have variable number of results.
 * the results may not on the top of stack.
 * for example:
 *  let a = 0;
 *  let b = 1;
 *  return a;
 * in this case, this function does not generate code that moves 'a' to top of stack.
 * */
size_t klgen_expr_inreturn(KlGenUnit* gen, KlAstExpr* ast, KlCodeVal* val);
/* try to generate code for expressions that can have variable number of results */
size_t klgen_takeall(KlGenUnit* gen, KlAstExpr* ast, KlCStkId target);
size_t klgen_passargs(KlGenUnit* gen, KlAstExprList* args);
static inline void klgen_expr_evaluated_to_noconst(KlGenUnit* gen, KlAstExpr* ast, KlCStkId target);


static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlAstExpr* ast) {
  KlCodeVal res = klgen_expr(gen, ast);
  klgen_putonstack(gen, &res, klgen_astposition(ast));
  return res;
}

static inline void klgen_expr_evaluated_to_noconst(KlGenUnit* gen, KlAstExpr* ast, KlCStkId target) {
  KlCodeVal res = klgen_exprtarget(gen, ast, target);
  if (klcodeval_isconstant(res))
    klgen_loadval(gen, target, res, klgen_astposition(ast));
  if (klgen_stacktop(gen) <= target)
    klgen_stackfree(gen, target + 1);
}

#endif
