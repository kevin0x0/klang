#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst.h"

static void klcst_exprunit_delete(KlCstExprUnit* unit);
static void klcst_exprbin_delete(KlCstExprBin* bin);
static void klcst_exprpre_delete(KlCstExprPre* pre);
static void klcst_exprpost_delete(KlCstExprPost* post);
static void klcst_exprtri_delete(KlCstExprTri* tri);

static KlCstVirtualFunc klcst_exprunit_vfunc = { .astdelete = (KlCstDelete)klcst_exprunit_delete };
static KlCstVirtualFunc klcst_exprbin_vfunc = { .astdelete = (KlCstDelete)klcst_exprbin_delete };
static KlCstVirtualFunc klcst_exprpre_vfunc = { .astdelete = (KlCstDelete)klcst_exprpre_delete };
static KlCstVirtualFunc klcst_exprpost_vfunc = { .astdelete = (KlCstDelete)klcst_exprpost_delete };
static KlCstVirtualFunc klcst_exprtri_vfunc = { .astdelete = (KlCstDelete)klcst_exprtri_delete };


KlCstExprUnit* klcst_exprunit_create(KlCstType type) {
  kl_assert(klcst_is_exprunit(type), "");

  KlCstExprUnit* unit = (KlCstExprUnit*)malloc(sizeof (KlCstExprUnit));
  if (kl_unlikely(!unit)) return NULL;
  klcst_init((KlCst*)unit, type, &klcst_exprunit_vfunc);
  return unit;
}

KlCstExprBin* klcst_exprbin_create(KlCstType type) {
  kl_assert(klcst_is_exprbin(type), "");

  KlCstExprBin* bin = (KlCstExprBin*)malloc(sizeof (KlCstExprBin));
  if (kl_unlikely(!bin)) return NULL;
  klcst_init((KlCst*)bin, type, &klcst_exprbin_vfunc);
  return bin;
}

KlCstExprPre* klcst_exprpre_create(KlCstType type) {
  kl_assert(klcst_is_exprpre(type), "");

  KlCstExprPre* pre = (KlCstExprPre*)malloc(sizeof (KlCstExprPre));
  if (kl_unlikely(!pre)) return NULL;
  klcst_init((KlCst*)pre, type, &klcst_exprpre_vfunc);
  return pre;
}

KlCstExprPost* klcst_exprpost_create(KlCstType type) {
  kl_assert(klcst_is_exprpost(type), "");

  KlCstExprPost* post = (KlCstExprPost*)malloc(sizeof (KlCstExprPost));
  if (kl_unlikely(!post)) return NULL;
  klcst_init((KlCst*)post, type, &klcst_exprpost_vfunc);
  return post;
}

KlCstExprTri* klcst_exprtri_create(KlCstType type) {
  kl_assert(klcst_is_exprtri(type), "");

  KlCstExprTri* tri = (KlCstExprTri*)malloc(sizeof (KlCstExprTri));
  if (kl_unlikely(!tri)) return NULL;
  klcst_init((KlCst*)tri, type, &klcst_exprtri_vfunc);
  return tri;
}

static void klcst_exprunit_delete(KlCstExprUnit* unit) {
  switch (klcst_type(klcast(KlCst*, unit))) {
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
      klcst_delete(unit->array.arrgen);
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
  klcst_delete(bin->loprand);
  klcst_delete(bin->roprand);
  free(bin);
}

static void klcst_exprpre_delete(KlCstExprPre* pre) {
  klcst_delete(pre->oprand);
  free(pre);
}

static void klcst_exprpost_delete(KlCstExprPost* post) {
  if (klcst_type(klcast(KlCst*, post)) == KLCST_EXPR_FUNC) {
    klcst_delete(post->func.block);
    free(post->func.params);
  } else {
    klcst_delete(post->other.oprand);
    klcst_delete(post->other.trailing);
  }
  free(post);
}

static void klcst_exprtri_delete(KlCstExprTri* tri) {
  klcst_delete(tri->cond);
  klcst_delete(tri->lexpr);
  klcst_delete(tri->rexpr);
  free(tri);
}
