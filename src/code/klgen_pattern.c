#include "include/code/klgen_pattern.h"
#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen.h"
#include "include/code/klgen_expr.h"
#include "include/code/klgen_exprbool.h"
#include "include/code/klgen_stmt.h"
#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/misc/klutils.h"
#include "include/parse/kltokens.h"




static void klgen_emit_tuplebinding(KlGenUnit* gen, size_t nval, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_arraybinding(KlGenUnit* gen, size_t nfront, size_t nback, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_mapbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_objbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_genericbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCIdx method, KlCStkId obj, size_t narg, KlFilePosition filepos);
static void klgen_emit_tuplematching(KlGenUnit* gen, size_t nval, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_arraymatching(KlGenUnit* gen, size_t nfront, size_t nback, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_mapmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_objmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
static void klgen_emit_genericmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCIdx method, KlCStkId obj, size_t narg, KlFilePosition filepos);

static void klgen_pattern_constant_matchjmp(KlGenUnit* gen, KlAstConstant* constant);
static void klgen_pattern_constant_match(KlGenUnit* gen, KlAstConstant* constant);

typedef struct tagKlGenPattern {
  void (*tuple)(KlGenUnit* gen, size_t nval, KlCStkId target, KlCStkId val, KlFilePosition filepos);
  void (*array)(KlGenUnit* gen, size_t nfront, size_t nback, KlCStkId target, KlCStkId val, KlFilePosition filepos);
  void (*map)(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
  void (*obj)(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos);
  void (*generic)(KlGenUnit* gen, size_t nres, KlCStkId target, KlCIdx method, KlCStkId obj, size_t narg, KlFilePosition filepos);
  void (*constant)(KlGenUnit* gen, KlAstConstant* constant);
} KlPatternEmitter;

static KlPatternEmitter binders = {
  .tuple = klgen_emit_tuplebinding,
  .array = klgen_emit_arraybinding,
  .map = klgen_emit_mapbinding,
  .obj = klgen_emit_objbinding,
  .generic = klgen_emit_genericbinding,
  .constant = klgen_pattern_constant_match,
};

static KlPatternEmitter matchers = {
  .tuple = klgen_emit_tuplematching,
  .array = klgen_emit_arraymatching,
  .map = klgen_emit_mapmatching,
  .obj = klgen_emit_objmatching,
  .generic = klgen_emit_genericmatching,
  .constant = klgen_pattern_constant_matchjmp,
};


static void klgen_pattern_fast(KlGenUnit* gen, KlAstExpr* pattern, KlPatternEmitter* emitter);
static KlCStkId klgen_pattern(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target, KlPatternEmitter* emitter);



static void klgen_emit_tuplebinding(KlGenUnit* gen, size_t nval, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbtup(target, val, nval), filepos);
}

static void klgen_emit_arraybinding(KlGenUnit* gen, size_t nfront, size_t nback, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbarr(target, val, nfront), filepos);
  klgen_emit(gen, klinst_parrextra(nback, 0), filepos);
}

static void klgen_emit_mapbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbmap(target, val, nres), filepos);
  klgen_emit(gen, klinst_pmappost(0), filepos);
}

static void klgen_emit_objbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pbobj(target, val, nres), filepos);
}

static void klgen_emit_genericbinding(KlGenUnit* gen, size_t nres, KlCStkId target, KlCIdx method, KlCStkId obj, size_t narg, KlFilePosition filepos) {
  klgen_emitmethod(gen, obj, method, narg, nres, target, filepos);
}

static void klgen_emit_tuplematching(KlGenUnit* gen, size_t nval, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmtup(target, val, nval), filepos);
  kl_assert(gen->jmpinfo.jumpinfo, "");
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist,
                                  klcodeval_jmplist(klgen_emit(gen, klinst_extra_xi(0, 0), filepos)));
}

static void klgen_emit_arraymatching(KlGenUnit* gen, size_t nfront, size_t nback, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmarr(target, val, nfront), filepos);
  kl_assert(gen->jmpinfo.jumpinfo, "");
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist,
                                  klcodeval_jmplist(klgen_emit(gen, klinst_parrextra(nback, 0), filepos)));
}

static void klgen_emit_mapmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmmap(target, val, nres), filepos);
  kl_assert(gen->jmpinfo.jumpinfo, "");
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist,
                                  klcodeval_jmplist(klgen_emit(gen, klinst_pmappost(0), filepos)));
}

static void klgen_emit_objmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCStkId val, KlFilePosition filepos) {
  klgen_emit(gen, klinst_pmobj(target, val, nres), filepos);
}

static void klgen_emit_genericmatching(KlGenUnit* gen, size_t nres, KlCStkId target, KlCIdx method, KlCStkId obj, size_t narg, KlFilePosition filepos) {
  klgen_emit(gen, klinst_hasfield(obj, method), filepos);
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist,
                                  klcodeval_jmplist(klgen_emit(gen, klinst_condjmp(KLC_FALSE, 0), filepos)));
  klgen_emitmethod(gen, obj, method, narg, nres, target, filepos);
}

static void klgen_pattern_constant_matchjmp(KlGenUnit* gen, KlAstConstant* constant) {
  KlCStkId leftop = klgen_stacktop(gen) - 1;
  KlFilePosition filepos = klgen_astposition(constant);
  if (constant->con.type == KLC_INT) {
    klgen_emit(gen, klinst_inrange(constant->con.intval, 16)
                    ? klinst_nei(leftop, constant->con.intval)
                    : klinst_nec(leftop, klgen_newinteger(gen, constant->con.intval)),
               filepos);
  } else {
    klgen_emit(gen, klinst_nec(leftop, klgen_newconstant(gen, &constant->con)), filepos);
  }
  KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), filepos);
  klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
  return;
}

static void klgen_pattern_constant_match(KlGenUnit* gen, KlAstConstant* constant) {
  KlCIdx conidx = klgen_newconstant(gen, &constant->con);
  klgen_emit(gen, klinst_match(klgen_stacktop(gen) - 1, conidx), klgen_astposition(constant));
}


/* if is an array with exact number of elements return false.
 * else return true and set pnfront and pnback. */
static void klgen_pattern_array_get_nfront_and_nback(KlGenUnit* gen, KlAstExpr** elems, size_t nelem, size_t* pnfront, size_t* pnback) {
  size_t nfront = 0;
  for (; nfront < nelem; ++nfront) {
    if (klast_kind(elems[nfront]) == KLAST_EXPR_VARARG)
      break;
  }
  size_t nback = 0;
  for (size_t i = nfront + 1; i < nelem; ++i) {
    if (klast_kind(elems[i]) != KLAST_EXPR_VARARG) {
      ++nback;
    } else {
      klgen_error(gen, klast_begin(elems[i]), klast_end(elems[i]), "duplicated '...' appeared in an array pattern");
    }
  }
  *pnfront = nfront;
  *pnback = nback;
}

static KlStrDesc klgen_pattern_methodname(KlGenUnit* gen, KlAstExpr* ast) {
  if (klast_kind(ast) == KLAST_EXPR_PRE) {
    KlTokenKind op = klcast(KlAstPre*, ast)->op;
    if (op == KLTK_MINUS) {
      return gen->strings->pattern_neg;
    } else {
      klgen_error(gen, klast_begin(ast), klast_end(ast), "unsupported pattern: '%s'", kltoken_desc(op));
      return gen->strings->pattern;
    }
  } else if (klast_kind(ast) == KLAST_EXPR_BIN) {
    KlTokenKind op = klcast(KlAstBin*, ast)->op;
    switch (op) {
      case KLTK_ADD: return gen->strings->pattern_add;
      case KLTK_MINUS: return gen->strings->pattern_sub;
      case KLTK_MUL: return gen->strings->pattern_mul;
      case KLTK_DIV: return gen->strings->pattern_div;
      case KLTK_IDIV: return gen->strings->pattern_idiv;
      case KLTK_MOD: return gen->strings->pattern_mod;
      case KLTK_CONCAT: return gen->strings->pattern_concat;
      default: {
        klgen_error(gen, klast_begin(ast), klast_end(ast), "unsupported pattern: '%s'", kltoken_desc(op));
        return gen->strings->pattern;
      }
    }
  } else if (klast_kind(ast) == KLAST_EXPR_CALL) {
    KlAstCall* call = klcast(KlAstCall*, ast);
    if (klast_kind(call->callable) != KLAST_EXPR_ID) {
      klgen_error(gen, klast_begin(call->callable), klast_end(call->callable), "should be an identifier in function call pattern");
      return gen->strings->pattern;
    }
    KlStrDesc callableid = klcast(KlAstIdentifier*, call->callable)->id;
    char* concat = klstrtbl_concat(gen->strtbl, gen->strings->pattern, callableid);
    klgen_oomifnull(gen, concat);
    KlStrDesc ret = { .id = klstrtbl_stringid(gen->strtbl, concat), .length = callableid.length + gen->strings->pattern.length };
    return ret;
  } else {
    klgen_error(gen, klast_begin(ast), klast_end(ast), "unsupported pattern");
    return gen->strings->pattern;
  }
}

static inline bool klgen_pattern_islval(KlAstExpr* lval) {
  return klast_expr_islvalue(lval);
}

static inline bool klgen_pattern_allislval(KlAstExpr** lvals, size_t nlval) {
  for (size_t i = 0; i < nlval; ++i) {
    if (!klgen_pattern_islval(lvals[i]))
      return false;
  }
  return true;
}

static inline bool klgen_pattern_allislval_maynull(KlAstExpr** lvals, size_t nlval) {
  for (size_t i = 0; i < nlval; ++i) {
    if (lvals[i] && !klgen_pattern_islval(lvals[i]))
      return false;
  }
  return true;
}

static KlCStkId klgen_pattern(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target, KlPatternEmitter* emitter) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      size_t nexpr = tuple->nval;
      KlAstExpr** exprs = tuple->vals;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      if (klgen_pattern_allislval(exprs, nexpr)) {
        emitter->tuple(gen, nexpr, target - nexpr, obj, klgen_astposition(tuple));
        klgen_stackfree(gen, obj);
        return target - nexpr;
      }
      /* else put on stack top */
      emitter->tuple(gen, nexpr, obj, obj, klgen_astposition(tuple));
      klgen_stackfree(gen, obj + nexpr);
      for (size_t i = nexpr; i--;)
        target = klgen_pattern(gen, exprs[i], target, emitter);
      return target;
    }
    case KLAST_EXPR_ARRAY: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      size_t nfront;
      size_t nback;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      klgen_pattern_array_get_nfront_and_nback(gen, elems, nelem, &nfront, &nback);
      if (klgen_pattern_allislval(elems, nfront) &&
          (nback == 0 || klgen_pattern_allislval(elems + nfront + 1, nelem - nfront - 1))) {
        emitter->array(gen, nfront, nback, target - (nfront + nback), obj, klgen_astposition(exprlist));
        klgen_stackfree(gen, obj);
        return target - (nfront + nback);
      }
      /* else put on stack top */
      emitter->array(gen, nfront, nback, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nfront + nback);
      if (nback != 0) {
        for (size_t i = nelem; i-- != nfront + 1;)
          target = klgen_pattern(gen, elems[i], target, emitter);
      }
      for (size_t i = nfront; i--;)
        target = klgen_pattern(gen, elems[i], target, emitter);
      return  target;
    }
    case KLAST_EXPR_CLASS: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      KlAstClassFieldDesc* fields = klclass->fields;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      size_t nshared = 0;
      for (size_t i = 0; i < nval; ++i) {
        if (!fields[i].shared) {
          klgen_error(gen, klast_begin(pattern), klast_end(pattern), "'local' can not appear in object pattern");
          continue;
        }
        ++nshared;
        KlCIdx name = klgen_newstring(gen, fields[i].name);
        klgen_emit(gen, klinst_loadc(klgen_stacktop(gen), name), klgen_astposition(pattern));
        klgen_stackalloc1(gen);
      }
      KlAstExpr** vals = klclass->vals;
      if (klgen_pattern_allislval_maynull(vals, nval)) {
        emitter->obj(gen, nshared, target - nshared, obj, klgen_astposition(pattern));
        klgen_stackfree(gen, obj);
        return target - nshared;
      }
      emitter->obj(gen, nshared, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nshared);
      for (size_t i = nval; i--;) {
        if (!fields[i].shared) continue;
        target = klgen_pattern(gen, vals[i], target, emitter);
      }
      return target;
    }
    case KLAST_EXPR_MAP: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstMap* map = klcast(KlAstMap*, pattern);
      KlAstExpr** vals = map->vals;
      KlAstExpr** keys = map->keys;
      size_t npair = map->npair;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      for (size_t i = 0; i < npair; ++i)
        klgen_exprtarget_noconst(gen, keys[i], klgen_stacktop(gen));
      klgen_stackalloc(gen, 2); /* need extra stack space in runtime */
      klgen_stackfree(gen, klgen_stacktop(gen) - 2);
      if (klgen_pattern_allislval(vals, npair)) {
        emitter->map(gen, npair, target - npair, obj, klgen_astposition(pattern));
        klgen_stackfree(gen, obj);
        return target - npair;
      }
      emitter->map(gen, npair, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + npair);
      for (size_t i = npair; i--;)
        target = klgen_pattern(gen, vals[i], target, emitter);
      return target;
    }
    case KLAST_EXPR_WALRUS: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstWalrus* walrus = klcast(KlAstWalrus*, pattern);
      KlCStkId copyobj = klgen_stackalloc1(gen);
      klgen_emitmove(gen, copyobj, copyobj - 1, 1, klgen_astposition(walrus));
      target = klgen_pattern(gen, walrus->rval, target, emitter);
      return klgen_pattern(gen, walrus->pattern, target, emitter);
    }
    case KLAST_EXPR_BIN: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlCStkId obj = klgen_stacktop(gen) - 1;
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      if (klgen_pattern_islval(bin->loperand) && klgen_pattern_islval(bin->roperand)) {
        emitter->generic(gen, 2, target - 2, method, obj, 0, klgen_astposition(pattern));
        klgen_stackfree(gen, obj);
        return target - 2;
      }
      emitter->generic(gen, 2, obj, method, obj, 0, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + 2);
      target = klgen_pattern(gen, bin->roperand, target, emitter);
      return klgen_pattern(gen, bin->loperand, target, emitter);
    }
    case KLAST_EXPR_PRE: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlCStkId obj = klgen_stacktop(gen) - 1;
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      if (klgen_pattern_islval(pre->operand)) {
        emitter->generic(gen, 2, target - 1, method, obj, 0, klgen_astposition(pattern));
        klgen_stackfree(gen, obj);
        return target - 1;
      }
      emitter->generic(gen, 1, obj, method, obj, 0, klgen_astposition(pattern));
      return klgen_pattern(gen, pre->operand, target, emitter);
    }
    case KLAST_EXPR_CALL: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlAstExprList* args = klcast(KlAstCall*, pattern)->args;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      size_t nelem = args->nexpr;
      KlAstExpr** elems = args->exprs;
      if (klgen_pattern_allislval(elems, nelem)) {
        emitter->generic(gen, args->nexpr, target - nelem, method, obj, 0, klgen_astposition(pattern));
        klgen_stackfree(gen, obj);
        return target - nelem;
      }
      emitter->generic(gen, args->nexpr, obj, method, obj, 0, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nelem);
      for (size_t i = nelem; i--;)
        target = klgen_pattern(gen, elems[i], target, emitter);
      return target;
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT:
    case KLAST_EXPR_WILDCARD:
    case KLAST_EXPR_ID: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlCStkId obj = klgen_stacktop(gen) - 1;
      klgen_emitmove(gen, target - 1, obj, 1, klgen_astposition(pattern));
      klgen_stackfree(gen, obj);
      return target - 1;
    }
    case KLAST_EXPR_CONSTANT: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      emitter->constant(gen, klcast(KlAstConstant*, pattern));
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return target;
    }
    case KLAST_EXPR_VARARG:
    default: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      klgen_error(gen, klast_begin(pattern), klast_end(pattern), "unsupported pattern");
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return target;
    }
  }
}

static void klgen_pattern_fast(KlGenUnit* gen, KlAstExpr* pattern, KlPatternEmitter* emitter) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      size_t nexpr = tuple->nval;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      emitter->tuple(gen, nexpr, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nexpr);
      if (nexpr != 0)
        klgen_pattern_fast(gen, tuple->vals[tuple->nval - 1], emitter);
      return;
    }
    case KLAST_EXPR_ARRAY: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      size_t nfront;
      size_t nback;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      klgen_pattern_array_get_nfront_and_nback(gen, elems, nelem, &nfront, &nback);
      emitter->array(gen, nfront, nback, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nfront + nback);
      for (size_t i = nelem; i--;) {
        if (klast_kind(elems[i]) != KLAST_EXPR_VARARG) {
          klgen_pattern_fast(gen, elems[i], emitter);
          return;
        }
      }
      return;
    }
    case KLAST_EXPR_CLASS: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      KlAstExpr** vals = klclass->vals;
      KlAstClassFieldDesc* fields = klclass->fields;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      size_t nshared = 0;
      KlAstExpr* lastshared = NULL;
      for (size_t i = 0; i < nval; ++i) {
        if (!fields[i].shared) {
          klgen_error(gen, klast_begin(pattern), klast_end(pattern), "'local' can not appeared in object pattern");
          continue;
        }
        ++nshared;
        KlCIdx name = klgen_newstring(gen, fields[i].name);
        klgen_emit(gen, klinst_loadc(klgen_stacktop(gen), name), klgen_astposition(pattern));
        klgen_stackalloc1(gen);
        lastshared = vals[i];
      }
      emitter->obj(gen, nshared, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nshared);
      if (nshared == 0) return;
      klgen_pattern_fast(gen, lastshared, emitter);
      return;
    }
    case KLAST_EXPR_MAP: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstMap* map = klcast(KlAstMap*, pattern);
      KlAstExpr** vals = map->vals;
      KlAstExpr** keys = map->keys;
      size_t npair = map->npair;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      for (size_t i = 0; i < npair; ++i)
        klgen_exprtarget_noconst(gen, keys[i], klgen_stacktop(gen));
      klgen_stackalloc(gen, 2); /* need extra stack space in runtime */
      klgen_stackfree(gen, klgen_stacktop(gen) - 2);
      emitter->map(gen, npair, obj, obj, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + npair);
      if (npair == 0) return;
      klgen_pattern_fast(gen, vals[npair - 1], emitter);
      return;
    }
    case KLAST_EXPR_WALRUS: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlAstWalrus* walrus = klcast(KlAstWalrus*, pattern);
      KlCStkId copyobj = klgen_stackalloc1(gen);
      klgen_emitmove(gen, copyobj, copyobj - 1, 1, klgen_astposition(walrus));
      klgen_pattern_fast(gen, walrus->rval, emitter);
      return;
    }
    case KLAST_EXPR_BIN: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlCStkId obj = klgen_stacktop(gen) - 1;
      emitter->generic(gen, 2, obj, method, obj, 0, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + 2);
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      klgen_pattern_fast(gen, bin->roperand, emitter);
      return;
    }
    case KLAST_EXPR_PRE: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlCStkId obj = klgen_stacktop(gen) - 1;
      emitter->generic(gen, 1, obj, method, obj, 0, klgen_astposition(pattern));
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      klgen_pattern_fast(gen, pre->operand, emitter);
      return;
    }
    case KLAST_EXPR_CALL: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      KlStrDesc methodname = klgen_pattern_methodname(gen, pattern);
      KlCIdx method = klgen_newstring(gen, methodname);
      KlAstCall* call = klcast(KlAstCall*, pattern);
      KlAstExprList* args = call->args;
      KlCStkId obj = klgen_stacktop(gen) - 1;
      size_t nelem = args->nexpr;
      emitter->generic(gen, args->nexpr, obj, method, obj, 0, klgen_astposition(pattern));
      klgen_stackfree(gen, obj + nelem);
      if (nelem == 0) return;
      klgen_pattern_fast(gen, args->exprs[nelem - 1], emitter);
      return;
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT:
    case KLAST_EXPR_WILDCARD:
    case KLAST_EXPR_ID: {
      return;
    }
    case KLAST_EXPR_VARARG: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      klgen_error(gen, klast_begin(pattern), klast_end(pattern), "unsupported pattern '...'");
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return;
    }
    case KLAST_EXPR_CONSTANT: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      emitter->constant(gen, klcast(KlAstConstant*, pattern));
      klgen_stackfree(gen, klgen_stacktop(gen) - 1);
      return;
    }
    default: {
      kl_assert(false, "unreachable");
      return;
    }
  }
}

static bool klgen_pattern_fast_check(KlGenUnit* gen, KlAstExpr* pattern) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      KlAstExpr** exprs = tuple->vals;
      size_t nexpr = tuple->nval;
      if (nexpr == 0) return true;
      return klgen_pattern_allislval(exprs, nexpr - 1) &&
             klgen_pattern_fast_check(gen, exprs[nexpr - 1]);
    }
    case KLAST_EXPR_ARRAY: {
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      if (nelem == 0) return true;
      KlAstExpr** elems = exprlist->exprs;
      size_t i = 0;
      for (; i < nelem; ++i) {
        if (!klgen_pattern_islval(elems[i]))
          break;
      }
      if (i == nelem) return true;
      if (klast_kind(elems[i]) == KLAST_EXPR_VARARG) {
        for (++i; i < nelem; ++i) {
          if (!klgen_pattern_islval(elems[i]))
            break;
        }
        if (i == nelem) return true;
      }
      if (i != nelem - 1) return false;
      return klgen_pattern_fast_check(gen, elems[i]);
    }
    case KLAST_EXPR_CLASS: {
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      if (nval == 0) return true;
      KlAstExpr** vals = klclass->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!vals[i]) continue;
        if (!klgen_pattern_islval(vals[i]))
          return false;
      }
      if (!vals[nval - 1]) return true;
      return klgen_pattern_fast_check(gen, vals[nval - 1]);
    }
    case KLAST_EXPR_MAP: {
      KlAstMap* map = klcast(KlAstMap*, pattern);
      size_t nval = map->npair;
      if (nval == 0) return true;
      KlAstExpr** vals = map->vals;
      for (size_t i = 0; i < nval - 1; ++i) {
        if (!klgen_pattern_islval(vals[i]))
          return false;
      }
      return klgen_pattern_fast_check(gen, vals[nval - 1]);
    }
    case KLAST_EXPR_WALRUS: {
      KlAstWalrus* walrus = klcast(KlAstWalrus*, pattern);
      if (!klgen_pattern_islval(walrus->pattern))
        return false;
      return klgen_pattern_fast_check(gen, walrus->rval);
    }
    case KLAST_EXPR_BIN: {
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      if (!klgen_pattern_islval(bin->loperand))
        return false;
      return klgen_pattern_fast_check(gen, bin->roperand);
    }
    case KLAST_EXPR_PRE: {
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      return klgen_pattern_fast_check(gen, pre->operand);
    }
    case KLAST_EXPR_CALL: {
      KlAstExprList* exprlist = klcast(KlAstCall*, pattern)->args;
      size_t nelem = exprlist->nexpr;
      if (nelem == 0) return true;
      KlAstExpr** elems = exprlist->exprs;
      for (size_t i = 0; i < nelem - 1; ++i) {
        if (!klgen_pattern_islval(elems[i]))
          return false;
      }
      return klgen_pattern_fast_check(gen, elems[nelem - 1]);
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT:
    case KLAST_EXPR_WILDCARD:
    case KLAST_EXPR_ID: {
      return true;
    }
    case KLAST_EXPR_VARARG: {
      return false;
    }
    case KLAST_EXPR_CONSTANT: {
      return true;
    }
    default: {
      return false;
    }
  }
}

void klgen_pattern_tostktop(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId val, KlPatternEmitter* emitter) {
  size_t stktop = klgen_stacktop(gen);
  if (klgen_pattern_fast_check(gen, pattern)) {
    kl_assert(stktop != val, "");
    klgen_emitmove(gen, stktop, val, 1, klgen_astposition(pattern));
    klgen_stackalloc1(gen);
    klgen_pattern_fast(gen, pattern, emitter);
  } else {
    size_t nres = klgen_pattern_count_result(gen, pattern);
    kl_assert(stktop + nres != val, "");
    klgen_emitmove(gen, stktop + nres, val, 1, klgen_astposition(pattern));
    klgen_stackalloc(gen, nres + 1);
    klgen_pattern(gen, pattern, stktop + nres, emitter);
  }
}

void klgen_pattern_binding_tostktop(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId val) {
  klgen_pattern_tostktop(gen, pattern, val, &binders);
}

void klgen_pattern_matching_tostktop(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId val) {
  klgen_pattern_tostktop(gen, pattern, val, &matchers);
}

size_t klgen_pattern_count_result(KlGenUnit* gen, KlAstExpr* pattern) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      size_t nexpr = tuple->nval;
      KlAstExpr** exprs = tuple->vals;
      size_t count = 0;
      for (size_t i = 0; i < nexpr; ++i)
        count += klgen_pattern_count_result(gen, exprs[i]);
      return count;
    }
    case KLAST_EXPR_ARRAY: {
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      size_t count = 0;
      for (size_t i = 0; i < nelem; ++i)
        count += klgen_pattern_count_result(gen, elems[i]);
      return count;
    }
    case KLAST_EXPR_CLASS: {
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      KlAstExpr** vals = klclass->vals;
      size_t count = 0;
      for (size_t i = 0; i < nval; ++i) {
        if (!vals[i]) continue;
        count += klgen_pattern_count_result(gen, vals[i]);
      }
      return count;
    }
    case KLAST_EXPR_MAP: {
      KlAstMap* map = klcast(KlAstMap*, pattern);
      KlAstExpr** vals = map->vals;
      size_t nval = map->npair;
      size_t count = 0;
      for (size_t i = 0; i < nval; ++i)
        count += klgen_pattern_count_result(gen, vals[i]);
      return count;
    }
    case KLAST_EXPR_WALRUS: {
      return klgen_pattern_count_result(gen, klcast(KlAstWalrus*, pattern)->rval) + 1;
    }
    case KLAST_EXPR_BIN: {
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      return klgen_pattern_count_result(gen, bin->loperand) +
             klgen_pattern_count_result(gen, bin->roperand);
    }
    case KLAST_EXPR_PRE: {
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      return klgen_pattern_count_result(gen, pre->operand);
    }
    case KLAST_EXPR_CALL: {
      KlAstExprList* exprlist = klcast(KlAstCall*, pattern)->args;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      size_t count = 0;
      for (size_t i = 0; i < nelem; ++i)
        count += klgen_pattern_count_result(gen, elems[i]);
      return count;
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT:
    case KLAST_EXPR_WILDCARD:
    case KLAST_EXPR_ID: {
      return 1;
    }
    case KLAST_EXPR_VARARG:
    case KLAST_EXPR_CONSTANT: {
      return 0;
    }
    default: {
      return 0;
    }
  }
}

void klgen_pattern_do_assignment(KlGenUnit* gen, KlAstExpr* pattern) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      size_t nexpr = tuple->nval;
      KlAstExpr** exprs = tuple->vals;
      for (size_t i = nexpr; i-- > 0;)
        klgen_pattern_do_assignment(gen, exprs[i]);
      break;
    }
    case KLAST_EXPR_ARRAY: {
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      for (size_t i = nelem; i-- > 0;)
        klgen_pattern_do_assignment(gen, elems[i]);
      break;
    }
    case KLAST_EXPR_CLASS: {
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      KlAstExpr** vals = klclass->vals;
      for (size_t i = nval; i-- > 0;) {
        if (!vals[i]) continue;
        klgen_pattern_do_assignment(gen, vals[i]);
      }
      break;
    }
    case KLAST_EXPR_MAP: {
      KlAstMap* map = klcast(KlAstMap*, pattern);
      KlAstExpr** vals = map->vals;
      size_t nval = map->npair;
      for (size_t i = nval; i-- > 0;)
        klgen_pattern_do_assignment(gen, vals[i]);
      break;
    }
    case KLAST_EXPR_WALRUS: {
      KlAstWalrus* walrus = klcast(KlAstWalrus*, pattern);
      klgen_pattern_do_assignment(gen, walrus->rval);
      klgen_pattern_do_assignment(gen, walrus->pattern);
      break;
    }
    case KLAST_EXPR_BIN: {
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      klgen_pattern_do_assignment(gen, bin->roperand);
      klgen_pattern_do_assignment(gen, bin->loperand);
      break;
    }
    case KLAST_EXPR_PRE: {
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      klgen_pattern_do_assignment(gen, pre->operand);
      break;
    }
    case KLAST_EXPR_CALL: {
      KlAstExprList* exprlist = klcast(KlAstCall*, pattern)->args;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      for (size_t i = nelem; i-- > 0;)
        klgen_pattern_do_assignment(gen, elems[i]);
      break;
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT:
    case KLAST_EXPR_WILDCARD:
    case KLAST_EXPR_ID: {
      kl_assert(klgen_stacktop(gen) > 0, "");
      size_t back = klgen_stacktop(gen) - 1;
      klgen_assignfrom(gen, pattern, back);
      klgen_stackfree(gen, back);
      break;
    }
    case KLAST_EXPR_VARARG:
    case KLAST_EXPR_CONSTANT: {
      break;
    }
    default: {
      break;
    }
  }
}

KlCStkId klgen_pattern_newsymbol(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId base) {
  switch (klast_kind(pattern)) {
    case KLAST_EXPR_TUPLE: {
      KlAstTuple* tuple = klcast(KlAstTuple*, pattern);
      size_t nexpr = tuple->nval;
      KlAstExpr** exprs = tuple->vals;
      for (size_t i = 0; i < nexpr; ++i)
        base = klgen_pattern_newsymbol(gen, exprs[i], base);
      return base;
    }
    case KLAST_EXPR_ARRAY: {
      KlAstExprList* exprlist = klcast(KlAstArray*, pattern)->exprlist;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      for (size_t i = 0; i < nelem; ++i)
        base = klgen_pattern_newsymbol(gen, elems[i], base);
      return base;
    }
    case KLAST_EXPR_CLASS: {
      KlAstClass* klclass = klcast(KlAstClass*, pattern);
      size_t nval = klclass->nfield;
      KlAstExpr** vals = klclass->vals;
      for (size_t i = 0; i < nval; ++i) {
        if (!vals[i]) continue;
        base = klgen_pattern_newsymbol(gen, vals[i], base);
      }
      return base;
    }
    case KLAST_EXPR_MAP: {
      KlAstMap* map = klcast(KlAstMap*, pattern);
      KlAstExpr** vals = map->vals;
      size_t nval = map->npair;
      for (size_t i = 0; i < nval; ++i)
        base = klgen_pattern_newsymbol(gen, vals[i], base);
      return base;
    }
    case KLAST_EXPR_WALRUS: {
      KlAstWalrus* walrus = klcast(KlAstWalrus*, pattern);
      base = klgen_pattern_newsymbol(gen, walrus->pattern, base);
      return klgen_pattern_newsymbol(gen, walrus->rval, base);
    }
    case KLAST_EXPR_BIN: {
      KlAstBin* bin = klcast(KlAstBin*, pattern);
      base = klgen_pattern_newsymbol(gen, bin->loperand, base);
      return klgen_pattern_newsymbol(gen, bin->roperand, base);
    }
    case KLAST_EXPR_PRE: {
      KlAstPre* pre = klcast(KlAstPre*, pattern);
      return klgen_pattern_newsymbol(gen, pre->operand, base);
    }
    case KLAST_EXPR_CALL: {
      KlAstExprList* exprlist = klcast(KlAstCall*, pattern)->args;
      size_t nelem = exprlist->nexpr;
      KlAstExpr** elems = exprlist->exprs;
      for (size_t i = 0; i < nelem; ++i)
        base = klgen_pattern_newsymbol(gen, elems[i], base);
      return base;
    }
    case KLAST_EXPR_INDEX:
    case KLAST_EXPR_DOT: {
      klgen_error(gen, klast_begin(pattern), klast_end(pattern), "expected an identifier");
      return base + 1;
    }
    case KLAST_EXPR_WILDCARD: {
      return base + 1;
    }
    case KLAST_EXPR_ID: {
      klgen_newsymbol(gen, klcast(KlAstIdentifier*, pattern)->id, base, klgen_astposition(pattern));
      return base + 1;
    }
    case KLAST_EXPR_VARARG:
    case KLAST_EXPR_CONSTANT: {
      return base;
    }
    default: {
      return base;
    }
  }
}



KlCStkId klgen_pattern_binding(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target) {
  return klgen_pattern(gen, pattern, target, &binders);
}

KlCStkId klgen_pattern_matching(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target) {
  return klgen_pattern(gen, pattern, target, &matchers);
}

bool klgen_pattern_fastbinding(KlGenUnit* gen, KlAstExpr* pattern) {
  if (!klgen_pattern_fast_check(gen, pattern))
    return false;
  klgen_pattern_fast(gen, pattern, &binders);
  return true;
}

bool klgen_pattern_fastmatching(KlGenUnit* gen, KlAstExpr* pattern) {
  if (!klgen_pattern_fast_check(gen, pattern))
    return false;
  klgen_pattern_fast(gen, pattern, &matchers);
  return true;
}
