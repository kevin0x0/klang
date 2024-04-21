#include "klang/include/code/klgen_pattern.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_stmt.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"


static void klgen_emit_tuplebinding(KlGenUnit* gen, size_t nres, size_t target) {
}




size_t klgen_pattern_binding(KlGenUnit* gen, KlCst* pattern, size_t target) {
}

size_t klgen_pattern_binding_tostktop(KlGenUnit* gen, KlCst* pattern, size_t val) {
}

size_t klgen_pattern_count_result(KlGenUnit* gen, KlCst* pattern) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      size_t count = 0;
      for (size_t i = 0; i < nelem; ++i)
        count += klgen_pattern_count_result(gen, elems[i]);
      return count;
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      size_t count = 0;
      for (size_t i = 0; i < nelem; ++i)
        count += klgen_pattern_count_result(gen, elems[i]);
      return count;
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      KlCst** vals = klclass->vals;
      size_t count = 0;
      for (size_t i = 0; i < nval; ++i) {
        if (!vals[i]) continue;
        count += klgen_pattern_count_result(gen, vals[i]);
      }
      return count;
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      KlCst** vals = map->vals;
      size_t nval = map->npair;
      size_t count = 0;
      for (size_t i = 0; i < nval; ++i)
        count += klgen_pattern_count_result(gen, vals[i]);
      return count;
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      return klgen_pattern_count_result(gen, bin->loperand) +
             klgen_pattern_count_result(gen, bin->roperand);
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      return klgen_pattern_count_result(gen, pre->operand);
    }
    case KLCST_EXPR_CALL: {
      KlCstCall* call = klcast(KlCstCall*, pattern);
      return klgen_pattern_count_result(gen, call->args);
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      if (post->op == KLTK_INDEX)
        return 1;
      return 0;
    }
    case KLCST_EXPR_DOT: {
      return 1;
    }
    case KLCST_EXPR_ID: {
      return 1;
    }
    case KLCST_EXPR_VARARG:
    case KLCST_EXPR_CONSTANT: {
      return 0;
    }
    default: {
      return 0;
    }
  }
}

static inline bool klgen_pattern_islval(KlCst* lval) {
  return (klcst_kind(lval) == KLCST_EXPR_ID   ||
          klcst_kind(lval) == KLCST_EXPR_DOT  ||
          (klcst_kind(lval) == KLCST_EXPR_POST &&
           klcast(KlCstPost*, lval)->op == KLTK_INDEX));
}

static bool klgen_pattern_fastbinding_check(KlGenUnit* gen, KlCst* pattern) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      if (nelem == 0) return true;
      KlCst** elems = tuple->elems;
      for (size_t i = 0; i < nelem - 1; ++i) {
        if (!klgen_pattern_islval(elems[i]))
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, elems[nelem - 1]);
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      if (nelem == 0) return true;
      KlCst** elems = tuple->elems;
      for (size_t i = 0; i < nelem - 1; ++i) {
        if (!klgen_pattern_islval(elems[i]) && klcst_kind(elems[i]) != KLCST_EXPR_VARARG)
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, elems[nelem - 1]);
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      if (nval == 0) return true;
      KlCst** vals = klclass->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!vals[i]) continue;
        if (!klgen_pattern_islval(vals[i]))
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, vals[nval - 1]);
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      size_t nval = map->npair;
      if (nval == 0) return true;
      KlCst** vals = map->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!klgen_pattern_islval(vals[i]))
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, vals[nval - 1]);
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      if (!klgen_pattern_islval(bin->loperand))
        return false;
      return klgen_pattern_fastbinding_check(gen, bin->roperand);
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      return klgen_pattern_fastbinding_check(gen, pre->operand);
    }
    case KLCST_EXPR_CALL: {
      KlCstCall* call = klcast(KlCstCall*, pattern);
      return klgen_pattern_fastbinding_check(gen, call->args);
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      return post->op == KLTK_INDEX;
    }
    case KLCST_EXPR_DOT: {
      return true;
    }
    case KLCST_EXPR_ID: {
      return true;
    }
    case KLCST_EXPR_VARARG: {
      return true;
    }
    case KLCST_EXPR_CONSTANT: {
      return true;
    }
    default: {
      return false;
    }
  }
}

static void klgen_pattern_do_fastbinding(KlGenUnit* gen, KlCst* pattern) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      if (nelem != 0)
        klgen_pattern_do_fastbinding(gen, elems[nelem - 1]);
      for (size_t i = 0; i < nelem - 1; ++i) {
        if (!klgen_pattern_islval(klgen_exprpromotion(elems[i])))
          return false;
      }
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      if (nelem == 0) return true;
      KlCst** elems = tuple->elems;
      for (size_t i = 0; i < nelem - 1; ++i) {
        KlCst* lval = klgen_exprpromotion(elems[i]);
        if (!klgen_pattern_islval(lval) && klcst_kind(lval) != KLCST_EXPR_VARARG)
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, elems[nelem - 1]);
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      if (nval == 0) return true;
      KlCst** vals = klclass->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!vals[i]) continue;
        if (!klgen_pattern_islval(klgen_exprpromotion(vals[i])))
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, vals[nval - 1]);
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      size_t nval = map->npair;
      if (nval == 0) return true;
      KlCst** vals = map->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!klgen_pattern_islval(klgen_exprpromotion(vals[i])))
          return false;
      }
      return klgen_pattern_fastbinding_check(gen, vals[nval - 1]);
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      if (!klgen_pattern_islval(klgen_exprpromotion(bin->loperand)))
        return false;
      return klgen_pattern_fastbinding_check(gen, bin->roperand);
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      return klgen_pattern_fastbinding_check(gen, pre->operand);
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      if (post->op == KLTK_CALL)
        return klgen_pattern_fastbinding_check(gen, post->post);
      return post->op == KLTK_INDEX;
    }
    case KLCST_EXPR_DOT: {
      return true;
    }
    case KLCST_EXPR_ID: {
      return true;
    }
    case KLCST_EXPR_VARARG:
    case KLCST_EXPR_CONSTANT: {
      return false;
    }
    default: {
      return false;
    }
  }
}

bool klgen_pattern_fastbinding(KlGenUnit* gen, KlCst* pattern) {
  if (!klgen_pattern_fastbinding_check(gen, pattern))
    return false;
  klgen_pattern_do_fastbinding(gen, pattern);
  return true;
}

void klgen_pattern_do_assignment(KlGenUnit* gen, KlCst* pattern) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      for (size_t i = nelem; i-- > 0;)
        klgen_pattern_do_assignment(gen, elems[i]);
      break;
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      for (size_t i = nelem; i-- > 0;)
        klgen_pattern_do_assignment(gen, elems[i]);
      break;
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      KlCst** vals = klclass->vals;
      for (size_t i = nval; i-- > 0;) {
        if (!vals[i]) continue;
        klgen_pattern_do_assignment(gen, vals[i]);
      }
      break;
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      KlCst** vals = map->vals;
      size_t nval = map->npair;
      for (size_t i = nval; i-- > 0;)
        klgen_pattern_do_assignment(gen, vals[i]);
      break;
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      klgen_pattern_do_assignment(gen, bin->roperand);
      klgen_pattern_do_assignment(gen, bin->loperand);
      break;
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      klgen_pattern_do_assignment(gen, pre->operand);
      break;
    }
    case KLCST_EXPR_CALL: {
      KlCstCall* call = klcast(KlCstCall*, pattern);
      klgen_pattern_do_assignment(gen, call->args);
      break;
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      if (post->op == KLTK_INDEX) {
        kl_assert(klgen_stacktop(gen) > 0, "");
        size_t back = klgen_stacktop(gen) - 1;
        klgen_assignfrom(gen, klcst(post), back);
        klgen_stackfree(gen, back);
      }
      break;
    }
    case KLCST_EXPR_DOT:
    case KLCST_EXPR_ID: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      size_t back = klgen_stacktop(gen) - 1;
      klgen_assignfrom(gen, pattern, back);
      klgen_stackfree(gen, back);
      break;
    }
    case KLCST_EXPR_VARARG:
    case KLCST_EXPR_CONSTANT: {
      break;
    }
    default: {
      break;
    }
  }
}

size_t klgen_pattern_newsymbol(KlGenUnit* gen, KlCst* pattern, size_t base) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      for (size_t i = 0; i < nelem; ++i)
        base = klgen_pattern_newsymbol(gen, elems[i], base);
      return base;
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      for (size_t i = 0; i < nelem; ++i)
        base = klgen_pattern_newsymbol(gen, elems[i], base);
      return base;
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      KlCst** vals = klclass->vals;
      for (size_t i = 0; i < nval; ++i) {
        if (!vals[i]) continue;
        base = klgen_pattern_newsymbol(gen, vals[i], base);
      }
      return base;
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      KlCst** vals = map->vals;
      size_t nval = map->npair;
      for (size_t i = 0; i < nval; ++i)
        base = klgen_pattern_newsymbol(gen, vals[i], base);
      return base;
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      base = klgen_pattern_newsymbol(gen, bin->loperand, base);
      return klgen_pattern_newsymbol(gen, bin->roperand, base);
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      return klgen_pattern_newsymbol(gen, pre->operand, base);
    }
    case KLCST_EXPR_CALL: {
      KlCstCall* call = klcast(KlCstCall*, pattern);
      return klgen_pattern_newsymbol(gen, call->args, base);
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      if (post->op == KLTK_INDEX)
        return base + 1;
      return base;
    }
    case KLCST_EXPR_DOT: {
      return base + 1;
    }
    case KLCST_EXPR_ID: {
      klgen_newsymbol(gen, klcast(KlCstIdentifier*, pattern)->id, base, klgen_cstposition(pattern));
      return base + 1;
    }
    case KLCST_EXPR_VARARG:
    case KLCST_EXPR_CONSTANT: {
      return base;
    }
    default: {
      return base;
    }
  }
}
