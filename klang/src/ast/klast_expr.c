#include "klang/include/ast/klast_expr.h"
#include "klang/include/ast/klast.h"

static void klast_exprunit_delete(KlAstExprUnit* unit);
static void klast_exprbin_delete(KlAstExprBin* bin);
static void klast_exprpre_delete(KlAstExprPre* pre);
static void klast_exprpost_delete(KlAstExprPost* post);
static void klast_exprtri_delete(KlAstExprTri* tri);

static KlAstVirtualFunc klast_exprunit_vfunc = { .astdelete = (KlAstDelete)klast_exprunit_delete };
static KlAstVirtualFunc klast_exprbin_vfunc = { .astdelete = (KlAstDelete)klast_exprbin_delete };
static KlAstVirtualFunc klast_exprpre_vfunc = { .astdelete = (KlAstDelete)klast_exprpre_delete };
static KlAstVirtualFunc klast_exprpost_vfunc = { .astdelete = (KlAstDelete)klast_exprpost_delete };
static KlAstVirtualFunc klast_exprtri_vfunc = { .astdelete = (KlAstDelete)klast_exprtri_delete };


KlAstExprUnit* klast_exprunit_create(KlAstType type) {
  kl_assert(klast_is_exprunit(type), "");

  KlAstExprUnit* unit = (KlAstExprUnit*)malloc(sizeof (KlAstExprUnit));
  if (kl_unlikely(!unit)) return NULL;
  klast_init((KlAst*)unit, type, &klast_exprunit_vfunc);
  return unit;
}

KlAstExprBin* klast_exprbin_create(KlAstType type) {
  kl_assert(klast_is_exprbin(type), "");

  KlAstExprBin* bin = (KlAstExprBin*)malloc(sizeof (KlAstExprBin));
  if (kl_unlikely(!bin)) return NULL;
  klast_init((KlAst*)bin, type, &klast_exprbin_vfunc);
  return bin;
}

KlAstExprPre* klast_exprpre_create(KlAstType type) {
  kl_assert(klast_is_exprpre(type), "");

  KlAstExprPre* pre = (KlAstExprPre*)malloc(sizeof (KlAstExprPre));
  if (kl_unlikely(!pre)) return NULL;
  klast_init((KlAst*)pre, type, &klast_exprpre_vfunc);
  return pre;
}

KlAstExprPost* klast_exprpost_create(KlAstType type) {
  kl_assert(klast_is_exprpost(type), "");

  KlAstExprPost* post = (KlAstExprPost*)malloc(sizeof (KlAstExprPost));
  if (kl_unlikely(!post)) return NULL;
  klast_init((KlAst*)post, type, &klast_exprpost_vfunc);
  return post;
}

KlAstExprTri* klast_exprtri_create(KlAstType type) {
  kl_assert(klast_is_exprtri(type), "");

  KlAstExprTri* tri = (KlAstExprTri*)malloc(sizeof (KlAstExprTri));
  if (kl_unlikely(!tri)) return NULL;
  klast_init((KlAst*)tri, type, &klast_exprtri_vfunc);
  return tri;
}

static void klast_exprunit_delete(KlAstExprUnit* unit) {
  switch (klast_type(klcast(KlAst*, unit))) {
    case KLAST_EXPR_TUPLE: {
      size_t nelem = unit->tuple.nelem;
      KlAst** elems = unit->tuple.elems;
      for (size_t i = 0; i < nelem; ++i)
        klast_delete(elems[i]);
      free(elems);
      break;
    }
    case KLAST_EXPR_MAP: {
      size_t npair = unit->map.npair;
      KlAst** keys = unit->map.keys;
      KlAst** vals = unit->map.vals;
      for (size_t i = 0; i < npair; ++i) {
        klast_delete(keys[i]);
        klast_delete(vals[i]);
      }
      free(keys);
      free(vals);
      break;
    }
    case KLAST_EXPR_ARR: {
      klast_delete(unit->array.arrgen);
      break;
    }
    case KLAST_EXPR_CLASS: {
      free(unit->klclass.fields);
      size_t nfield = unit->klclass.nfield;
      KlAst** vals = unit->klclass.vals;
      for (size_t i = 0; i < nfield; ++i) {
        if (vals[i]) klast_delete(vals[i]);
      }
      break;
    }
    case KLAST_EXPR_CONSTANT:
    case KLAST_EXPR_ID: {
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
  free(unit);
}

static void klast_exprbin_delete(KlAstExprBin* bin) {
  klast_delete(bin->loprand);
  klast_delete(bin->roprand);
  free(bin);
}

static void klast_exprpre_delete(KlAstExprPre* pre) {
  klast_delete(pre->oprand);
  free(pre);
}

static void klast_exprpost_delete(KlAstExprPost* post) {
  if (klast_type(klcast(KlAst*, post)) == KLAST_EXPR_FUNC) {
    klast_delete(post->func.block);
    free(post->func.params);
  } else {
    klast_delete(post->other.oprand);
    klast_delete(post->other.trailing);
  }
  free(post);
}

static void klast_exprtri_delete(KlAstExprTri* tri) {
  klast_delete(tri->cond);
  klast_delete(tri->lexpr);
  klast_delete(tri->rexpr);
  free(tri);
}
