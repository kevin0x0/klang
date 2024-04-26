#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H

#include "klang/include/code/klcode.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klgen.h"
#include "klang/include/vm/klinst.h"



KlCodeVal klgen_expr(KlGenUnit* gen, KlCst* cst);
KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlCst* cst, size_t target);
void klgen_exprlist_raw(KlGenUnit* gen, KlCst** csts, size_t ncst, size_t nwanted, KlFilePosition filepos);
static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlCst* cst, size_t target);
static inline KlCodeVal klgen_tuple(KlGenUnit* gen, KlCstTuple* tuplecst);
static inline KlCodeVal klgen_tuple_target(KlGenUnit* gen, KlCstTuple* tuplecst, size_t target);
static inline void klgen_expryield(KlGenUnit* gen, KlCstYield* yieldcst, size_t nwanted);
void klgen_multival(KlGenUnit* gen, KlCst* cst, size_t nval, size_t target);
/* try to generate code for expressions that can have variable number of results.
 * the results may not on the top of stack.
 * for example:
 *  let a = 0;
 *  let b = 1;
 *  return a;
 * in this case, this function does not generate code that moves 'a' to top of stack.
 * */
size_t klgen_trytakeall(KlGenUnit* gen, KlCst* cst, KlCodeVal* val);
/* try to generate code for expressions that can have variable number of results */
size_t klgen_takeall(KlGenUnit* gen, KlCst* cst, size_t target);
size_t klgen_passargs(KlGenUnit* gen, KlCst* args);
KlCodeVal klgen_exprpost(KlGenUnit* gen, KlCstPost* postcst, size_t target, bool append_target);
KlCodeVal klgen_exprpre(KlGenUnit* gen, KlCstPre* precst, size_t target);
KlCodeVal klgen_exprbin(KlGenUnit* gen, KlCstBin* bincst, size_t target);
void klgen_exprarr(KlGenUnit* gen, KlCstArray* arrcst, size_t target);
void klgen_exprarrgen(KlGenUnit* gen, KlCstArrayGenerator* arrgencst, size_t target);
void klgen_exprmap(KlGenUnit* gen, KlCstMap* mapcst, size_t target);
void klgen_exprclass(KlGenUnit* gen, KlCstClass* classcst, size_t target);


static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlCst* cst) {
  KlCodeVal res = klgen_expr(gen, cst);
  klgen_putonstack(gen, &res, klgen_cstposition(cst));
  return res;
}

static inline void klgen_exprtarget_noconst(KlGenUnit* gen, KlCst* cst, size_t target) {
  KlCodeVal res = klgen_exprtarget(gen, cst, target);
  if (klcodeval_isconstant(res))
    klgen_loadval(gen, target, res, klgen_cstposition(cst));
  if (klgen_stacktop(gen) <= target)
    klgen_stackfree(gen, target + 1);
}

static inline KlCodeVal klgen_tuple(KlGenUnit* gen, KlCstTuple* tuplecst) {
  if (tuplecst->nelem == 0)
    return klcodeval_nil();

  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem - 1;
  size_t oristktop = klgen_stacktop(gen);
  while (expr != end) {
    klgen_expr(gen, *expr++);
    klgen_stackfree(gen, oristktop);
  }
  return klgen_expr(gen, *expr);
}

static inline KlCodeVal klgen_tuple_target(KlGenUnit* gen, KlCstTuple* tuplecst, size_t target) {
  if (tuplecst->nelem == 0)
    return klcodeval_nil();

  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem - 1;
  size_t oristktop = klgen_stacktop(gen);
  while (expr != end) {
    klgen_expr(gen, *expr++);
    klgen_stackfree(gen, oristktop);
  }
  return klgen_exprtarget(gen, *expr, target);
}

static inline void klgen_expryield(KlGenUnit* gen, KlCstYield* yieldcst, size_t nwanted) {
  size_t base = klgen_stacktop(gen);
  size_t nres = klgen_passargs(gen, yieldcst->vals);
  klgen_emit(gen, klinst_yield(base, nres, nwanted), klgen_cstposition(yieldcst));
  if (nwanted != KLINST_VARRES)
    klgen_stackfree(gen, base + nwanted);
}


#endif
