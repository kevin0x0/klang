#include "klang/include/code/klgen_pattern.h"
#include "klang/include/code/klcode.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_stmt.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klstrtbl.h"
#include "klang/include/parse/kltokens.h"
#include "klang/include/vm/klinst.h"
#include <strings.h>


static inline void klgen_emit_exactarraybinding(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbtup(target, val, nres), filepos);
}

static inline void klgen_emit_arraybinding(KlGenUnit* gen, size_t nfront, size_t nback, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbarr(target, val, nfront), filepos);
  klgen_emit(gen, klinst_parrextra(nback, 0), filepos);
}

static inline void klgen_emit_mapbinding(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbmap(target, val, nres), filepos);
  klgen_emit(gen, klinst_pmappost(0), filepos);
}

static inline void klgen_emit_objbinding(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbobj(target, val, nres), filepos);
}

static inline void klgen_emit_genericbinding(KlGenUnit* gen, size_t nres, size_t target, size_t method, size_t obj, size_t narg, KlFilePosition filepos) {
  klgen_emitmethod(gen, obj, method, narg, nres, target, filepos);
}

static inline size_t klgen_emit_exactarraymatching(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmtup(target, val, nres), filepos);
  return klgen_emit(gen, klinst_extra_xi(0, 0), filepos);
}

static inline size_t klgen_emit_arraymatching(KlGenUnit* gen, size_t nfront, size_t nback, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmarr(target, val, nfront), filepos);
  return klgen_emit(gen, klinst_parrextra(nback, 0), filepos);
}

static inline size_t klgen_emit_mapmatching(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmmap(target, val, nres), filepos);
  return klgen_emit(gen, klinst_pmappost(0), filepos);
}

static inline void klgen_emit_objmatching(KlGenUnit* gen, size_t nres, size_t target, size_t val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmobj(target, val, nres), filepos);
}

static inline size_t klgen_emit_genericmatch(KlGenUnit* gen, size_t nres, size_t target, size_t method, size_t obj, size_t narg, KlFilePosition filepos) {
  klgen_emit(gen, klinst_hasfield(obj, method), filepos);
  size_t pc = klgen_emit(gen, klinst_condjmp(KL_TRUE, 0), filepos);
  klgen_emitmethod(gen, obj, method, narg, nres, target, filepos);
  return pc;
}


/* if is an array with exact number of elements return false.
 * else return true and set pnfront and pnback. */
static bool klgen_pattern_array_get_nfront_and_nback(KlGenUnit* gen, KlCst** elems, size_t nelem, size_t* pnfront, size_t* pnback) {
  size_t nfront = 0;
  for (; nfront < nelem; ++nfront) {
    if (klcst_kind(elems[nfront]) == KLCST_EXPR_VARARG)
      break;
  }
  if (nfront == nelem) return false;
  size_t nback = 0;
  for (size_t i = nfront + 1; i < nelem; ++i) {
    if (klcst_kind(elems[i]) == KLCST_EXPR_VARARG) {
      klgen_error(gen, klcst_begin(elems[i]), klcst_end(elems[i]), "duplicated '...' appeared in same array pattern");
      continue;
    }
    ++nback;
  }
  nback = nelem - nfront;
  *pnfront = nfront;
  *pnback = nback;
  return true;
}

static KlStrDesc klgen_pattern_methodname(KlGenUnit* gen, KlCst* cst) {
  if (klcst_kind(cst) == KLCST_EXPR_PRE) {
    KlTokenKind op = klcast(KlCstPre*, cst)->op;
    if (op == KLTK_MINUS) {
      return gen->strings->pattern_neg;
    } else {
      klgen_error(gen, klcst_begin(cst), klcst_end(cst), "unsupported pattern: '%s'", kltoken_desc(op));
      return gen->strings->pattern;
    }
  } else if (klcst_kind(cst) == KLCST_EXPR_BIN) {
    KlTokenKind op = klcast(KlCstBin*, cst)->op;
    switch (op) {
      case KLTK_ADD: return gen->strings->pattern_add;
      case KLTK_MINUS: return gen->strings->pattern_sub;
      case KLTK_MUL: return gen->strings->pattern_mul;
      case KLTK_DIV: return gen->strings->pattern_div;
      case KLTK_IDIV: return gen->strings->pattern_idiv;
      case KLTK_MOD: return gen->strings->pattern_mod;
      case KLTK_CONCAT: return gen->strings->pattern_concat;
      default: {
        klgen_error(gen, klcst_begin(cst), klcst_end(cst), "unsupported pattern: '%s'", kltoken_desc(op));
        return gen->strings->pattern;
      }
    }
  } else if (klcst_kind(cst) == KLCST_EXPR_CALL) {
    KlCstCall* call = klcast(KlCstCall*, cst);
    if (klcst_kind(call->callable) != KLTK_ID) {
      klgen_error(gen, klcst_begin(call->callable), klcst_end(call->callable), "should be an identifier in pattern binding and pattern matching");
      return gen->strings->pattern;
    }
    KlStrDesc callableid = klcast(KlCstIdentifier*, call->callable)->id;
    char* concat = klstrtbl_concat(gen->strtbl, gen->strings->pattern, callableid);
    klgen_oomifnull(gen, concat);
    KlStrDesc ret = { .id = klstrtbl_stringid(gen->strtbl, concat), .length = callableid.length + gen->strings->pattern.length };
    return ret;
  } else {
    klgen_error(gen, klcst_begin(cst), klcst_end(cst), "unsupported pattern");
    return gen->strings->pattern;
  }
}

static inline bool klgen_pattern_islval(KlCst* lval) {
  return (klcst_kind(lval) == KLCST_EXPR_ID   ||
          klcst_kind(lval) == KLCST_EXPR_DOT  ||
          (klcst_kind(lval) == KLCST_EXPR_POST &&
           klcast(KlCstPost*, lval)->op == KLTK_INDEX));
}

static inline bool klgen_pattern_allislval(KlCst** lvals, size_t nlval) {
  for (size_t i = 0; i < nlval; ++i) {
    if (!klgen_pattern_islval(lvals[i]))
      return false;
  }
  return true;
}

static inline bool klgen_pattern_allislval_maynull(KlCst** lvals, size_t nlval) {
  for (size_t i = 0; i < nlval; ++i) {
    if (lvals[i] && !klgen_pattern_islval(lvals[i]))
      return false;
  }
  return true;
}

static inline bool klgen_pattern_allislvalorvararg(KlCst** lvals, size_t nlval) {
  for (size_t i = 0; i < nlval; ++i) {
    if (!klgen_pattern_islval(lvals[i]) && klcst_kind(lvals[i]) != KLCST_EXPR_VARARG)
      return false;
  }
  return true;
}

static void klgen_pattern_constant_match(KlGenUnit* gen, KlCstConstant* constant) {
  (void)klgen_pattern_constant_match;
}

static void klgen_pattern_constant_mustmatch(KlGenUnit* gen, KlCstConstant* constant) {
  size_t conidx = klgen_newconstant(gen, &constant->con);
  klgen_emit(gen, klinst_match(klgen_stacktop(gen) - 1, conidx), klgen_cstposition(constant));
}

size_t klgen_pattern_binding(KlGenUnit* gen, KlCst* pattern, size_t target) {
  switch (klcst_kind(pattern)) {
    case KLCST_EXPR_TUPLE: {
      KlCstTuple* tuple = klcast(KlCstTuple*, pattern);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      size_t obj = klgen_stacktop(gen) - 1;
      if (klgen_pattern_allislval(elems, nelem)) {
        klgen_emit_exactarraybinding(gen, nelem, target - nelem, obj, klgen_cstposition(tuple));
        klgen_stackfree(gen, obj);
        return target - nelem;
      }
      /* else put on stack top */
      klgen_emit_exactarraybinding(gen, nelem, obj, obj, klgen_cstposition(tuple));
      klgen_stackfree(gen, obj + nelem);
      for (size_t i = nelem; i--;)
        target = klgen_pattern_binding(gen, elems[i], target);
      return target;
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      size_t nfront;
      size_t nback;
      size_t obj = klgen_stacktop(gen) - 1;
      if (klgen_pattern_array_get_nfront_and_nback(gen, elems, nelem, &nfront, &nback)) {
        if (klgen_pattern_allislvalorvararg(elems, nelem)) {
          klgen_emit_arraybinding(gen, nfront, nback, target - (nfront + nback), obj, klgen_cstposition(tuple));
          klgen_stackfree(gen, obj);
          return target - (nfront + nback);
        }
        /* else put on stack top */
        klgen_emit_arraybinding(gen, nfront, nback, obj, obj, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj + nfront + nback);
        for (size_t i = nelem; i--;)
          target = klgen_pattern_binding(gen, elems[i], target);
        return  target;
      } else {
        if (klgen_pattern_allislval(elems, nelem)) {
          klgen_emit_exactarraybinding(gen, nelem, target - nelem, obj, klgen_cstposition(tuple));
          klgen_stackfree(gen, obj);
          return target - nelem;
        }
        /* else put on stack top */
        klgen_emit_exactarraybinding(gen, nelem, obj, obj, klgen_cstposition(tuple));
        klgen_stackfree(gen, obj + nelem);
        for (size_t i = nelem; i--;)
          target = klgen_pattern_binding(gen, elems[i], target);
        return target;
      }
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      KlCstClassFieldDesc* fields = klclass->fields;
      size_t obj = klgen_stacktop(gen) - 1;
      size_t nshared = 0;
      for (size_t i = 0; i < nval; ++i) {
        if (!fields[i].shared) {
          klgen_error(gen, klcst_begin(pattern), klcst_end(pattern), "'local' can not appeared in object pattern");
          continue;
        }
        ++nshared;
        size_t name = klgen_newstring(gen, fields[i].name);
        klgen_emit(gen, klinst_loadc(klgen_stacktop(gen), name), klgen_cstposition(pattern));
        klgen_stackalloc1(gen);
      }
      KlCst** vals = klclass->vals;
      if (klgen_pattern_allislval_maynull(vals, nval)) {
        klgen_emit_objbinding(gen, nshared, target - nshared, obj, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj);
        return target - nshared;
      }
      klgen_emit_objbinding(gen, nshared, obj, obj, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + nshared);
      for (size_t i = nval; i--;) {
        if (!fields[i].shared) continue;
        target = klgen_pattern_binding(gen, vals[i], target);
      }
      return target;
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      KlCst** vals = map->vals;
      KlCst** keys = map->keys;
      size_t npair = map->npair;
      size_t obj = klgen_stacktop(gen) - 1;
      for (size_t i = 0; i < npair; ++i)
        klgen_exprtarget_noconst(gen, keys[i], klgen_stacktop(gen));
      klgen_stackalloc(gen, 2); /* need extra stack space in runtime */
      klgen_stackfree(gen, klgen_stacktop(gen) - 2);
      if (klgen_pattern_allislval(vals, npair)) {
        klgen_emit_mapbinding(gen, npair, target - npair, obj, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj);
        return target - npair;
      }
      for (size_t i = npair; i--;)
        target = klgen_pattern_binding(gen, vals[i], target);
      return target;
    }
    case KLCST_EXPR_BIN: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      size_t obj = klgen_stacktop(gen) - 1;
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      if (klgen_pattern_islval(bin->loperand) && klgen_pattern_islval(bin->roperand)) {
        klgen_emit_genericbinding(gen, 2, target - 2, method, obj, 0, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj);
        return target - 2;
      }
      klgen_emit_genericbinding(gen, 2, obj, method, obj, 0, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + 2);
      target = klgen_pattern_binding(gen, bin->roperand, target);
      return klgen_pattern_binding(gen, bin->loperand, target);
    }
    case KLCST_EXPR_PRE: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      size_t obj = klgen_stacktop(gen) - 1;
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      if (klgen_pattern_islval(pre->operand)) {
        klgen_emit_genericbinding(gen, 2, target - 1, method, obj, 0, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj);
        return target - 1;
      }
      klgen_emit_genericbinding(gen, 1, obj, method, obj, 0, klgen_cstposition(pattern));
      return klgen_pattern_binding(gen, pre->operand, target);
    }
    case KLCST_EXPR_CALL: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      KlCstCall* call = klcast(KlCstCall*, pattern);
      KlCstTuple* args = klcast(KlCstTuple*, call->args);
      size_t obj = klgen_stacktop(gen) - 1;
      size_t nelem = args->nelem;
      KlCst** elems = args->elems;
      if (klgen_pattern_allislval(elems, nelem)) {
        klgen_emit_genericbinding(gen, args->nelem, target - nelem, method, obj, 0, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj);
        return target - nelem;
      }
      klgen_emit_genericbinding(gen, args->nelem, obj, method, obj, 0, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + nelem);
      for (size_t i = nelem; i--;)
        target = klgen_pattern_binding(gen, elems[i], target);
      return target;
    }
    case KLCST_EXPR_POST: {
      KlCstPost* post = klcast(KlCstPost*, pattern);
      if (post->op != KLTK_INDEX) {
        klgen_error(gen, klcst_begin(pattern), klcst_end(pattern), "unsupported pattern");
        return target;
      }
      size_t obj = klgen_stacktop(gen) - 1;
      klgen_emitmove(gen, target - 1, obj, 1, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj);
      return target - 1;
    }
    case KLCST_EXPR_DOT:
    case KLCST_EXPR_ID: {
      size_t obj = klgen_stacktop(gen) - 1;
      klgen_emitmove(gen, target - 1, obj, 1, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj);
      return target - 1;
    }
    case KLCST_EXPR_CONSTANT: {
      klgen_pattern_constant_mustmatch(gen, klcast(KlCstConstant*, pattern));
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return target - 1;
    }
    case KLCST_EXPR_VARARG:
    default: {
      klgen_error(gen, klcst_begin(pattern), klcst_end(pattern), "unsupported pattern '...'");
      return target - 1;
    }
  }
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
      size_t i = 0;
      KlCst* lastvalid = NULL;
      for (; i < nelem; ++i) {
        if (klcst_kind(elems[i]) == KLCST_EXPR_VARARG)
          continue;
        lastvalid = elems[i];
        if (!klgen_pattern_islval(elems[i]))
          break;
      }
      for (++i; i < nelem; ++i) {
        if (klcst_kind(elems[i]) != KLCST_EXPR_VARARG)
          return false;
      }
      return lastvalid ? klgen_pattern_fastbinding_check(gen, lastvalid) : true;
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
      if (!vals[nval - 1]) return true;
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
      size_t obj = klgen_stacktop(gen) - 1;
      klgen_emit_exactarraybinding(gen, nelem, obj, obj, klgen_cstposition(tuple));
      klgen_stackfree(gen, obj + nelem);
      if (nelem == 0) return;
      klgen_pattern_do_fastbinding(gen, elems[nelem - 1]);
      return;
    }
    case KLCST_EXPR_ARR: {
      KlCstTuple* tuple = klcast(KlCstTuple*, klcast(KlCstArray*, pattern)->vals);
      size_t nelem = tuple->nelem;
      KlCst** elems = tuple->elems;
      size_t nfront;
      size_t nback;
      size_t obj = klgen_stacktop(gen) - 1;
      if (klgen_pattern_array_get_nfront_and_nback(gen, elems, nelem, &nfront, &nback)) {
        klgen_emit_arraybinding(gen, nfront, nback, obj, obj, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj + nfront + nback);
        for (size_t i = nelem; i--;) {
          if (klcst_kind(elems[i]) != KLCST_EXPR_VARARG) {
            klgen_pattern_do_fastbinding(gen, elems[i]);
            return;
          }
        }
      } else {
        klgen_emit_exactarraybinding(gen, nelem, obj, obj, klgen_cstposition(pattern));
        klgen_stackfree(gen, obj + nelem);
        if (nelem == 0) return;
        klgen_pattern_do_fastbinding(gen, elems[nelem - 1]);
      }
      return;
    }
    case KLCST_EXPR_CLASS: {
      KlCstClass* klclass = klcast(KlCstClass*, pattern);
      size_t nval = klclass->nfield;
      KlCst** vals = klclass->vals;
      KlCstClassFieldDesc* fields = klclass->fields;
      size_t obj = klgen_stacktop(gen) - 1;
      size_t nshared = 0;
      KlCst* lastshared = NULL;
      for (size_t i = 0; i < nval; ++i) {
        if (!fields[i].shared) {
          klgen_error(gen, klcst_begin(pattern), klcst_end(pattern), "'local' can not appeared in object pattern");
          continue;
        }
        ++nshared;
        size_t name = klgen_newstring(gen, fields[i].name);
        klgen_emit(gen, klinst_loadc(klgen_stacktop(gen), name), klgen_cstposition(pattern));
        klgen_stackalloc1(gen);
        lastshared = vals[i];
      }
      klgen_emit_objbinding(gen, nshared, obj, obj, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + nshared);
      if (nshared == 0) return;
      klgen_pattern_do_fastbinding(gen, lastshared);
      return;
    }
    case KLCST_EXPR_MAP: {
      KlCstMap* map = klcast(KlCstMap*, pattern);
      KlCst** vals = map->vals;
      KlCst** keys = map->keys;
      size_t npair = map->npair;
      size_t obj = klgen_stacktop(gen) - 1;
      for (size_t i = 0; i < npair; ++i)
        klgen_exprtarget_noconst(gen, keys[i], klgen_stacktop(gen));
      klgen_stackalloc(gen, 2); /* need extra stack space in runtime */
      klgen_stackfree(gen, klgen_stacktop(gen) - 2);
      klgen_emit_mapbinding(gen, npair, obj, obj, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + npair);
      if (npair == 0) return;
      klgen_pattern_do_fastbinding(gen, vals[npair - 1]);
      return;
    }
    case KLCST_EXPR_BIN: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      size_t obj = klgen_stacktop(gen) - 1;
      klgen_emit_genericbinding(gen, 2, obj, method, obj, 0, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + 2);
      KlCstBin* bin = klcast(KlCstBin*, pattern);
      klgen_pattern_do_fastbinding(gen, bin->roperand);
      return;
    }
    case KLCST_EXPR_PRE: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      size_t obj = klgen_stacktop(gen) - 1;
      klgen_emit_genericbinding(gen, 1, obj, method, obj, 0, klgen_cstposition(pattern));
      KlCstPre* pre = klcast(KlCstPre*, pattern);
      klgen_pattern_do_fastbinding(gen, pre->operand);
      return;
    }
    case KLCST_EXPR_CALL: {
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      size_t method = klgen_newstring(gen, methodname);
      KlCstCall* call = klcast(KlCstCall*, pattern);
      KlCstTuple* args = klcast(KlCstTuple*, call->args);
      size_t obj = klgen_stacktop(gen) - 1;
      size_t nelem = args->nelem;
      klgen_emit_genericbinding(gen, args->nelem, obj, method, obj, 0, klgen_cstposition(pattern));
      klgen_stackfree(gen, obj + nelem);
      if (nelem == 0) return;
      klgen_pattern_do_fastbinding(gen, args->elems[nelem - 1]);
      return;
    }
    case KLCST_EXPR_POST: {
      kl_assert(klcast(KlCstPost*, pattern)->op == KLTK_INDEX, "");
      return;
    }
    case KLCST_EXPR_DOT: {
      return;
    }
    case KLCST_EXPR_ID: {
      return;
    }
    case KLCST_EXPR_VARARG: {
      klgen_error(gen, klcst_begin(pattern), klcst_end(pattern), "unsupported pattern '...'");
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return;
    }
    case KLCST_EXPR_CONSTANT: {
      klgen_pattern_constant_mustmatch(gen, klcast(KlCstConstant*, pattern));
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return;
    }
    default: {
      kl_assert(false, "unreachable");
      return;
    }
  }
}

void klgen_pattern_binding_tostktop(KlGenUnit* gen, KlCst* pattern, size_t val) {
  size_t stktop = klgen_stacktop(gen);
  if (klgen_pattern_fastbinding_check(gen, pattern)) {
    klgen_emitmove(gen, stktop, val, 1, klgen_cstposition(gen));
    klgen_stackalloc1(gen);
    klgen_pattern_do_fastbinding(gen, pattern);
  } else {
    size_t nres = klgen_pattern_count_result(gen, pattern);
    klgen_emitmove(gen, stktop + nres, val, 1, klgen_cstposition(gen));
    klgen_stackalloc(gen, nres + 1);
    klgen_pattern_binding(gen, pattern, stktop + nres);
  }
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
