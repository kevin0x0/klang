#ifndef _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_
#define _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_

#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/lang/klinst.h"



KlCodeVal klgen_expr(KlGenUnit* gen, KlAst* ast);
KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlAst* ast, KlCStkId target);
void klgen_exprlist_raw(KlGenUnit* gen, KlAst** asts, size_t nast, size_t nwanted, KlFilePosition filepos);
void klgen_multival(KlGenUnit* gen, KlAst* ast, size_t nval, KlCStkId target);
/* try to generate code for expressions that can have variable number of results.
 * the results may not on the top of stack.
 * for example:
 *  let a = 0;
 *  let b = 1;
 *  return a;
 * in this case, this function does not generate code that moves 'a' to top of stack.
 * */
size_t klgen_trytakeall(KlGenUnit* gen, KlAst* ast, KlCodeVal* val);
/* try to generate code for expressions that can have variable number of results */
size_t klgen_takeall(KlGenUnit* gen, KlAst* ast, KlCStkId target);
size_t klgen_passargs(KlGenUnit* gen, KlAstExprList* args);
static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlAst* ast, KlCStkId target);


static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  KlCodeVal res = klgen_exprtarget(gen, ast, target);
  if (klcodeval_isconstant(res))
    klgen_loadval(gen, target, res, klgen_astposition(ast));
  if (klgen_stacktop(gen) <= target)
    klgen_stackfree(gen, target + 1);
}

#endif
