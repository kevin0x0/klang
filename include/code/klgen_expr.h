#ifndef _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_
#define _KLANG_INCLUDE_CODE_KLGEN_EXPR_H_

#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/lang/klinst.h"



KlCodeVal klgen_expr(KlGenUnit* gen, KlAst* ast);
KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlAst* ast, KlCStkId target);
void klgen_exprlist_raw(KlGenUnit* gen, KlAst** asts, size_t nast, size_t nwanted, KlFilePosition filepos);
static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlAst* ast, KlCStkId target);
static inline KlCodeVal klgen_tuple(KlGenUnit* gen, KlAstTuple* tupleast);
static inline KlCodeVal klgen_tuple_target(KlGenUnit* gen, KlAstTuple* tupleast, KlCStkId target);
static inline void klgen_expryield(KlGenUnit* gen, KlAstYield* yieldast, size_t nwanted);
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
size_t klgen_passargs(KlGenUnit* gen, KlAstTuple* args);
KlCodeVal klgen_exprpost(KlGenUnit* gen, KlAstPost* postast, KlCStkId target, bool append_target);
KlCodeVal klgen_exprpre(KlGenUnit* gen, KlAstPre* preast, KlCStkId target);
KlCodeVal klgen_exprbin(KlGenUnit* gen, KlAstBin* binast, KlCStkId target);
void klgen_exprarr(KlGenUnit* gen, KlAstArray* arrast, KlCStkId target);
void klgen_exprarrgen(KlGenUnit* gen, KlAstArrayGenerator* arrgenast, KlCStkId target);
void klgen_exprmap(KlGenUnit* gen, KlAstMap* mapast, KlCStkId target);
void klgen_exprclass(KlGenUnit* gen, KlAstClass* classast, KlCStkId target);


static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlAst* ast) {
  KlCodeVal res = klgen_expr(gen, ast);
  klgen_putonstack(gen, &res, klgen_astposition(ast));
  return res;
}

static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  KlCodeVal res = klgen_exprtarget(gen, ast, target);
  if (klcodeval_isconstant(res))
    klgen_loadval(gen, target, res, klgen_astposition(ast));
  if (klgen_stacktop(gen) <= target)
    klgen_stackfree(gen, target + 1);
}

static inline KlCodeVal klgen_tuple(KlGenUnit* gen, KlAstTuple* tupleast) {
  if (tupleast->nelem == 0)
    return klcodeval_nil();

  KlAst** expr = tupleast->elems;
  KlAst** end = expr + tupleast->nelem - 1;
  KlCStkId oristktop = klgen_stacktop(gen);
  while (expr != end) {
    klgen_expr(gen, *expr++);
    klgen_stackfree(gen, oristktop);
  }
  return klgen_expr(gen, *expr);
}

static inline KlCodeVal klgen_tuple_target(KlGenUnit* gen, KlAstTuple* tupleast, KlCStkId target) {
  if (tupleast->nelem == 0)
    return klcodeval_nil();

  KlAst** expr = tupleast->elems;
  KlAst** end = expr + tupleast->nelem - 1;
  KlCStkId oristktop = klgen_stacktop(gen);
  while (expr != end) {
    klgen_expr(gen, *expr++);
    klgen_stackfree(gen, oristktop);
  }
  return klgen_exprtarget(gen, *expr, target);
}

static inline void klgen_expryield(KlGenUnit* gen, KlAstYield* yieldast, size_t nwanted) {
  KlCStkId base = klgen_stacktop(gen);
  size_t nres = klgen_passargs(gen, yieldast->vals);
  klgen_emit(gen, klinst_yield(base, nres, nwanted), klgen_astposition(yieldast));
  if (nwanted != KLINST_VARRES)
    klgen_stackfree(gen, base + nwanted);
}


#endif
