#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H

#include "klang/include/code/klgen.h"


KlCodeVal klgen_expr(KlGenUnit* gen, KlCst* cst);
KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlCst* cst, size_t target);
static inline KlCodeVal klgen_tuple_as_singleval(KlGenUnit* gen, KlCstTuple* tuplecst);
static inline KlCodeVal klgen_tuple_as_singleval_target(KlGenUnit* gen, KlCstTuple* tuplecst, size_t target);
static inline void klgen_tuple_evaluate(KlGenUnit* gen, KlCstTuple* tuplecst, size_t ndiscard);
static inline void klgen_expryield(KlGenUnit* gen, KlCstYield* yieldcst, size_t nwanted);
/* generate code that evaluates expressions on the tuple and put their values in the top of stack.
 * nwanted is the number of expected values. */
void klgen_tuple(KlGenUnit* gen, KlCstTuple* tuplecst, size_t nwanted);
size_t klgen_passargs(KlGenUnit* gen, KlCst* args);
KlCodeVal klgen_exprpost(KlGenUnit* gen, KlCstPost* postcst);
KlCodeVal klgen_exprpre(KlGenUnit* gen, KlCstPre* precst);
KlCodeVal klgen_exprbin(KlGenUnit* gen, KlCstBin* bincst);
KlCodeVal klgen_constant(KlGenUnit* gen, KlCstConstant* concst);
KlCodeVal klgen_identifier(KlGenUnit* gen, KlCstIdentifier* idcst);
void klgen_exprarr(KlGenUnit* gen, KlCstArray* arrcst, size_t target);
void klgen_exprarrgen(KlGenUnit* gen, KlCstArrayGenerator* arrgencst, size_t target);
void klgen_exprmap(KlGenUnit* gen, KlCstMap* mapcst, size_t target);
void klgen_exprclass(KlGenUnit* gen, KlCstClass* classcst, size_t target);


static inline KlCodeVal klgen_tuple_as_singleval(KlGenUnit* gen, KlCstTuple* tuplecst) {
  if (tuplecst->nelem == 0)
    return klcodeval_nil();

  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem - 1;
  while (expr != end)
    klgen_expr(gen, *expr++);
  return klgen_expr(gen, *expr);
}

static inline KlCodeVal klgen_tuple_as_singleval_target(KlGenUnit* gen, KlCstTuple* tuplecst, size_t target) {
  if (tuplecst->nelem == 0)
    return klcodeval_nil();

  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem - 1;
  while (expr != end)
    klgen_expr(gen, *expr++);
  return klgen_exprtarget(gen, *expr, target);
}

static inline void klgen_tuple_evaluate(KlGenUnit* gen, KlCstTuple* tuplecst, size_t ndiscard) {
  kl_assert(ndiscard <= tuplecst->nelem, "");
  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + ndiscard;
  while (expr != end)
    klgen_expr(gen, *expr++);
}

static inline void klgen_expryield(KlGenUnit* gen, KlCstYield* yieldcst, size_t nwanted) {
  size_t stkid = klgen_stacktop(gen);
  size_t nres = klgen_passargs(gen, yieldcst->vals);
  klgen_pushinst(gen, klinst_yield(stkid, nres, nwanted), klgen_cstposition(yieldcst));
  klgen_stackfree(gen, stkid);
}


#endif
