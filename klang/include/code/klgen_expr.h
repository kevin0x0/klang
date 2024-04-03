#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_EXPR_H

#include "klang/include/code/klcodeval.h"
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
void klgen_multival(KlGenUnit* gen, KlCst* cst, size_t nval);
size_t klgen_passargs(KlGenUnit* gen, KlCst* args);
void klgen_exprpost(KlGenUnit* gen, KlCstPost* postcst, size_t target);
KlCodeVal klgen_exprpre(KlGenUnit* gen, KlCstPre* precst, size_t target);
KlCodeVal klgen_exprbin(KlGenUnit* gen, KlCstBin* bincst, size_t target);
void klgen_exprarr(KlGenUnit* gen, KlCstArray* arrcst, size_t target);
void klgen_exprarrgen(KlGenUnit* gen, KlCstArrayGenerator* arrgencst, size_t target);
void klgen_exprmap(KlGenUnit* gen, KlCstMap* mapcst, size_t target);
void klgen_exprclass(KlGenUnit* gen, KlCstClass* classcst, size_t target);


static inline KlCodeVal klgen_tuple_as_singleval(KlGenUnit* gen, KlCstTuple* tuplecst) {
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

static inline KlCodeVal klgen_tuple_as_singleval_target(KlGenUnit* gen, KlCstTuple* tuplecst, size_t target) {
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

static inline void klgen_tuple_evaluate(KlGenUnit* gen, KlCstTuple* tuplecst, size_t ndiscard) {
  kl_assert(ndiscard <= tuplecst->nelem, "");
  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + ndiscard;
  size_t oristktop = klgen_stacktop(gen);
  while (expr != end) {
    klgen_expr(gen, *expr++);
    klgen_stackfree(gen, oristktop);
  }
}

static inline void klgen_expryield(KlGenUnit* gen, KlCstYield* yieldcst, size_t nwanted) {
  size_t base = klgen_stacktop(gen);
  size_t nres = klgen_passargs(gen, yieldcst->vals);
  klgen_pushinst(gen, klinst_yield(base, nres, nwanted), klgen_cstposition(yieldcst));
  klgen_stackfree(gen, base + nwanted);
}


#endif
