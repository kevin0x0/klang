#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst.h"

static void klcst_exprunit_delete(KlCstExprUnit* unit);
static void klcst_exprbin_delete(KlCstExprBin* bin);
static void klcst_exprpre_delete(KlCstExprPre* pre);
static void klcst_exprpost_delete(KlCstExprPost* post);
static void klcst_exprter_delete(KlCstExprTer* ter);

static KlCstVirtualFunc klcst_exprunit_vfunc = { .cstdelete = (KlCstDelete)klcst_exprunit_delete };
static KlCstVirtualFunc klcst_exprbin_vfunc = { .cstdelete = (KlCstDelete)klcst_exprbin_delete };
static KlCstVirtualFunc klcst_exprpre_vfunc = { .cstdelete = (KlCstDelete)klcst_exprpre_delete };
static KlCstVirtualFunc klcst_exprpost_vfunc = { .cstdelete = (KlCstDelete)klcst_exprpost_delete };
static KlCstVirtualFunc klcst_exprter_vfunc = { .cstdelete = (KlCstDelete)klcst_exprter_delete };


KlCstExprUnit* klcst_exprunit_create(KlCstKind type) {
  kl_assert(klcst_is_exprunit(type), "");

  KlCstExprUnit* unit = (KlCstExprUnit*)malloc(sizeof (KlCstExprUnit));
  if (kl_unlikely(!unit)) return NULL;
  klcst_init((KlCst*)unit, type, &klcst_exprunit_vfunc);
  return unit;
}

KlCstExprBin* klcst_exprbin_create(KlTokenKind op) {
  KlCstExprBin* bin = (KlCstExprBin*)malloc(sizeof (KlCstExprBin));
  if (kl_unlikely(!bin)) return NULL;
  klcst_init((KlCst*)bin, KLCST_EXPR_BIN, &klcst_exprbin_vfunc);
  bin->binop = op;
  return bin;
}

KlCstExprPre* klcst_exprpre_create(KlCstKind type) {
  kl_assert(klcst_is_exprpre(type), "");

  KlCstExprPre* pre = (KlCstExprPre*)malloc(sizeof (KlCstExprPre));
  if (kl_unlikely(!pre)) return NULL;
  klcst_init((KlCst*)pre, type, &klcst_exprpre_vfunc);
  return pre;
}

KlCstExprPost* klcst_exprpost_create(KlCstKind type) {
  kl_assert(klcst_is_exprpost(type), "");

  KlCstExprPost* post = (KlCstExprPost*)malloc(sizeof (KlCstExprPost));
  if (kl_unlikely(!post)) return NULL;
  klcst_init((KlCst*)post, type, &klcst_exprpost_vfunc);
  return post;
}

KlCstExprTer* klcst_exprter_create(void) {
  KlCstExprTer* tri = (KlCstExprTer*)malloc(sizeof (KlCstExprTer));
  if (kl_unlikely(!tri)) return NULL;
  klcst_init((KlCst*)tri, KLCST_EXPR_TER, &klcst_exprter_vfunc);
  return tri;
}

static void klcst_exprunit_delete(KlCstExprUnit* unit) {
  switch (klcst_kind(klcast(KlCst*, unit))) {
    case KLCST_EXPR_TUPLE: {
      size_t nelem = unit->tuple.nelem;
      KlCst** elems = unit->tuple.elems;
      for (size_t i = 0; i < nelem; ++i)
        klcst_delete(elems[i]);
      free(elems);
      break;
    }
    case KLCST_EXPR_MAP: {
      size_t npair = unit->map.npair;
      KlCst** keys = unit->map.keys;
      KlCst** vals = unit->map.vals;
      for (size_t i = 0; i < npair; ++i) {
        klcst_delete(keys[i]);
        klcst_delete(vals[i]);
      }
      free(keys);
      free(vals);
      break;
    }
    case KLCST_EXPR_ARR: {
      klcst_delete(unit->array.exprs);
      if (unit->array.stmts)
        klcst_delete(unit->array.stmts);
      break;
    }
    case KLCST_EXPR_CLASS: {
      free(unit->klclass.fields);
      size_t nfield = unit->klclass.nfield;
      KlCst** vals = unit->klclass.vals;
      for (size_t i = 0; i < nfield; ++i) {
        if (vals[i]) klcst_delete(vals[i]);
      }
      break;
    }
    case KLCST_EXPR_CONSTANT:
    case KLCST_EXPR_ID: {
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  free(unit);
}

static void klcst_exprbin_delete(KlCstExprBin* bin) {
  klcst_delete(bin->loperand);
  klcst_delete(bin->roperand);
  free(bin);
}

static void klcst_exprpre_delete(KlCstExprPre* pre) {
  klcst_delete(pre->operand);
  if (klcst_kind(klcast(KlCst*, pre)) == KLCST_EXPR_NEW)
    klcst_delete(pre->params);
  free(pre);
}

static void klcst_exprpost_delete(KlCstExprPost* post) {
  if (klcst_kind(klcast(KlCst*, post)) == KLCST_EXPR_FUNC) {
    klcst_delete(post->func.block);
    free(post->func.params);
  } else if (klcst_kind(klcast(KlCst*, post)) == KLCST_EXPR_INDEX) {
    klcst_delete(post->index.indexable);
    klcst_delete(post->index.index);
  } else if (klcst_kind(klcast(KlCst*, post)) == KLCST_EXPR_DOT) {
    klcst_delete(post->dot.operand);
  } else if (klcst_kind(klcast(KlCst*, post)) == KLCST_EXPR_CALL) {
    klcst_delete(post->call.callable);
    klcst_delete(post->call.param);
  }
  free(post);
}

static void klcst_exprter_delete(KlCstExprTer* ter) {
  klcst_delete(ter->cond);
  klcst_delete(ter->lexpr);
  klcst_delete(ter->rexpr);
  free(ter);
}

void klcst_expr_tuple_delete_after_stolen(KlCst* tuple) {
  kl_assert(klcst_kind(tuple) == KLCST_EXPR_TUPLE, "");
  KlCstExprUnit* incomplete = klcast(KlCstExprUnit*, tuple);
  size_t nelem = incomplete->tuple.nelem;
  KlCst** elems = incomplete->tuple.elems;
  for (size_t i = 0; i < nelem; ++i)
    if (elems[i]) klcst_delete(elems[i]);
  free(elems);
}
