#include "include/code/klcontbl.h"
#include "include/code/klgen.h"
#include "include/code/klcodeval.h"
#include "include/code/klgen_expr.h"
#include "include/code/klgen_pattern.h"
#include "include/code/klgen_exprbool.h"
#include "include/code/klgen_stmt.h"
#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "include/lang/klinst.h"
#include <setjmp.h>



static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlAst* ast);
static inline KlCodeVal klgen_exprlist(KlGenUnit* gen, KlAstExprList* exprlist);
static inline KlCodeVal klgen_exprlist_target(KlGenUnit* gen, KlAstExprList* exprlist, KlCStkId target);
static inline void klgen_expryield(KlGenUnit* gen, KlAstYield* yieldast, size_t nwanted);
static KlCodeVal klgen_exprpost(KlGenUnit* gen, KlAstPost* postast, KlCStkId target, bool append_target);
static KlCodeVal klgen_exprpre(KlGenUnit* gen, KlAstPre* preast, KlCStkId target);
static KlCodeVal klgen_exprbin(KlGenUnit* gen, KlAstBin* binast, KlCStkId target);
static size_t klgen_exprwhere(KlGenUnit* gen, KlAstWhere* whereast, size_t nwanted, KlCStkId target);
static void klgen_exprmatch(KlGenUnit* gen, KlAstMatch* matchast, KlCStkId target, bool needresult);
static void klgen_exprnew(KlGenUnit* gen, KlAstNew* newast, KlCStkId target);
static void klgen_exprarr(KlGenUnit* gen, KlAstArray* arrast, KlCStkId target);
static void klgen_exprarrgen(KlGenUnit* gen, KlAstArrayGenerator* arrgenast, KlCStkId target);
static void klgen_exprmap(KlGenUnit* gen, KlAstMap* mapast, KlCStkId target);
static void klgen_exprclass(KlGenUnit* gen, KlAstClass* classast, KlCStkId target);
static KlCodeVal klgen_exprbinleftliteral(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCStkId target);
static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCodeVal right, KlCStkId target, KlCStkId oristktop);


static inline KlCodeVal klgen_expr_onstack(KlGenUnit* gen, KlAst* ast) {
  KlCodeVal res = klgen_expr(gen, ast);
  klgen_putonstack(gen, &res, klgen_astposition(ast));
  return res;
}

static inline KlCodeVal klgen_exprlist(KlGenUnit* gen, KlAstExprList* exprlist) {
  if (exprlist->nexpr == 0)
    return klcodeval_nil();
  klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr - 1, 0, klgen_astposition(exprlist));
  return klgen_expr(gen, exprlist->exprs[exprlist->nexpr - 1]);
}

static inline KlCodeVal klgen_exprlist_target(KlGenUnit* gen, KlAstExprList* exprlist, KlCStkId target) {
  if (exprlist->nexpr == 0)
    return klcodeval_nil();
  klgen_exprlist_raw(gen, exprlist->exprs, exprlist->nexpr - 1, 0, klgen_astposition(exprlist));
  return klgen_exprtarget(gen, exprlist->exprs[exprlist->nexpr - 1], target);
}

static inline void klgen_expryield(KlGenUnit* gen, KlAstYield* yieldast, size_t nwanted) {
  KlCStkId base = klgen_stacktop(gen);
  size_t nres = klgen_passargs(gen, yieldast->vals);
  klgen_emit(gen, klinst_yield(base, nres, nwanted), klgen_astposition(yieldast));
  if (nwanted != KLINST_VARRES)
    klgen_stackfree(gen, base + nwanted);
}



static void klgen_exprarr(KlGenUnit* gen, KlAstArray* arrast, KlCStkId target) {
  KlCStkId argbase = klgen_stacktop(gen);
  size_t nval = klgen_passargs(gen, arrast->exprlist);
  klgen_emit(gen, klinst_mkarray(target, argbase, nval), klgen_astposition(arrast));
  klgen_stackfree(gen, target == argbase ? target + 1 : argbase);
}

static void klgen_exprarrgen(KlGenUnit* gen, KlAstArrayGenerator* arrgenast, KlCStkId target) {
  KlCStkId stktop = klgen_stackalloc1(gen);
  klgen_emit(gen, klinst_mkarray(stktop, stktop, 0), klgen_astposition(arrgenast));
  klgen_newsymbol(gen, arrgenast->arrid, stktop, klgen_position(klast_begin(arrgenast), klast_begin(arrgenast)));
  bool needclose = klgen_stmtblockpure(gen, arrgenast->block);
  if (needclose)
    klgen_emit(gen, klinst_close(stktop), klgen_astposition(arrgenast));
  if (target != stktop) {
    klgen_emitmove(gen, target, stktop, 1, klgen_astposition(arrgenast));
    klgen_stackfree(gen, stktop);
  }
}

static void klgen_exprvararg(KlGenUnit* gen, KlAstVararg* varargast, size_t nwanted, KlCStkId target) {
  if (!gen->vararg) {
    klgen_error(gen, klast_begin(varargast), klast_end(varargast),
                "not inside a variable parameter function. "
                "Did you forget to add '...' at the end of the parameter list?");
  }
  KlCStkId stktop = klgen_stacktop(gen);
  klgen_emit(gen, klinst_vararg(target, nwanted), klgen_astposition(varargast));
  if (nwanted != KLINST_VARRES)
    klgen_stackfree(gen, stktop > target + nwanted ? stktop : target + nwanted);
}

static bool klgen_exprhasverres(KlGenUnit* gen, KlAst* expr) {
  switch (klast_kind(expr)) {
    case KLAST_EXPR_YIELD: {
      return true;
    }
    case KLAST_EXPR_CALL: {
      return true;
    }
    case KLAST_EXPR_WHERE: {
      return klgen_exprhasverres(gen, klcast(KlAstWhere*, expr)->expr);
    }
    case KLAST_EXPR_VARARG: {
      return true;
    }
    default: {
      return false;
    }
  }
}

static void klgen_expr_domatching(KlGenUnit* gen, KlAst* pattern, KlCStkId base, KlCStkId matchobj) {
  if (klast_kind(pattern) == KLAST_EXPR_CONSTANT) {
    KlConstant* constant = &klcast(KlAstConstant*, pattern)->con;
    KlFilePosition filepos = klgen_astposition(pattern);
    if (constant->type == KLC_INT) {
      klgen_emit(gen, klinst_inrange(constant->intval, 16)
                      ? klinst_nei(matchobj, constant->intval)
                      : klinst_nec(matchobj, klgen_newinteger(gen, constant->intval)),
                 filepos);
    } else {
      klgen_emit(gen, klinst_nec(matchobj, klgen_newconstant(gen, constant)), filepos);
    }
    KlCPC pc = klgen_emit(gen, klinst_condjmp(true, 0), filepos);
    klgen_mergejmplist_maynone(gen, &gen->jmpinfo.jumpinfo->terminatelist, klcodeval_jmplist(pc));
  } else {
    klgen_pattern_matching_tostktop(gen, pattern, matchobj);
    klgen_pattern_newsymbol(gen, pattern, base);
  }
}

static void klgen_exprmatch(KlGenUnit* gen, KlAstMatch* matchast, KlCStkId target, bool needresult) {
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal matchobj = klgen_expr(gen, matchast->matchobj);
  klgen_putonstack(gen, &matchobj, klgen_astposition(matchast->matchobj));
  KlCStkId currstktop = klgen_stacktop(gen);
  /* reserve this position to avoid destroying potentially referenced variables. */
  if (currstktop == target && needresult)
    currstktop = klgen_stackalloc1(gen);

  KlCodeVal jmpoutlist = klcodeval_none();
  size_t npattern = matchast->npattern;
  KlAst** patterns = matchast->patterns;
  KlAst** exprs = matchast->exprs;
  for (size_t i = 0; i < npattern; ++i) {
    KlGenJumpInfo jmpinfo = {
      .truelist = klcodeval_none(),
      .falselist = klcodeval_none(),
      .terminatelist = klcodeval_none(),
      .prev = gen->jmpinfo.jumpinfo,
    };
    gen->jmpinfo.jumpinfo = &jmpinfo;

    klgen_pushsymtbl(gen);
    klgen_expr_domatching(gen, patterns[i], currstktop, matchobj.index);
    needresult ? klgen_exprtarget_noconst(gen, exprs[i], target)
               : klgen_multival(gen, exprs[i], 0, klgen_stacktop(gen));


    if (i != npattern - 1 || needresult) {
      KlCPC pc = klgen_emit(gen, gen->symtbl->info.referenced ? klinst_closejmp(currstktop, 0)
                                                              : klinst_jmp(0),
                            klgen_astposition(matchast));
      klgen_mergejmplist_maynone(gen, &jmpoutlist, klcodeval_jmplist(pc));
    } else if (gen->symtbl->info.referenced) {
      klgen_emit(gen, klinst_close(currstktop), klgen_astposition(matchast));
    }
    klgen_popsymtbl(gen);

    klgen_setinstjmppos(gen, jmpinfo.terminatelist, klgen_currentpc(gen));
    gen->jmpinfo.jumpinfo = jmpinfo.prev;
    kl_assert(jmpinfo.truelist.kind == KLVAL_NONE && jmpinfo.falselist.kind == KLVAL_NONE, "");
    klgen_stackfree(gen, currstktop);
  }
  if (needresult)
    klgen_emitloadnils(gen, target, 1, klgen_astposition(matchast));
  klgen_setinstjmppos(gen, jmpoutlist, klgen_currentpc(gen));
  klgen_stackfree(gen, oristktop == target ? target + 1 : oristktop);
}

static size_t klgen_exprwhere(KlGenUnit* gen, KlAstWhere* whereast, size_t nwanted, KlCStkId target) {
  size_t oristktop = klgen_stacktop(gen);
  if (nwanted == KLINST_VARRES && !klgen_exprhasverres(gen, klcast(KlAstWhere*, whereast)->expr))
    nwanted = 1;
  /* for variable results we should evaluate the results to stack top
   * to avoid destroying potentially referenced variables. */
  if (nwanted == KLINST_VARRES) {
    klgen_pushsymtbl(gen);
    klgen_stmtlist(gen, whereast->block);
    KlCStkId stktop = klgen_stacktop(gen);
    size_t nres = klgen_takeall(gen, whereast->expr, stktop);
    if (gen->symtbl->info.referenced)
      klgen_emit(gen, klinst_close(oristktop), klgen_astposition(whereast));
    klgen_popsymtbl(gen);
    /* move results to 'target' */
    if (stktop != target)
      klgen_emitmove(gen, target, stktop, nres, klgen_astposition(whereast->expr));
    if (nres != KLINST_VARRES) {
      klgen_stackfree(gen, oristktop > target + nres ? oristktop : target + nres);
    } else {
      klgen_stackfree(gen, oristktop);
    }
    return nres;
  }
  /* else we reserve enough space to avoid destroying potentially
   * referenced variables. */
  if (target + nwanted > oristktop)
    klgen_stackalloc(gen, target + nwanted - oristktop);
  klgen_pushsymtbl(gen);
  klgen_stmtlist(gen, whereast->block);
  /* no symbol is referenced, so we evaluate the expression directly to its 'target' */
  klgen_multival(gen, whereast->expr, nwanted, target);
  if (gen->symtbl->info.referenced)
    klgen_emit(gen, klinst_close(oristktop), klgen_astposition(whereast));
  /* evaluation is complete */
  klgen_popsymtbl(gen);
  klgen_stackfree(gen, oristktop > target + nwanted ? oristktop : target + nwanted);
  return nwanted;
}

/* deconstruct parameters */
static void klgen_exprfunc_deconstruct_params(KlGenUnit* gen, KlAstExprList* funcparams) {
  kl_assert(klgen_stacktop(gen) == 0, "");
  size_t npattern = funcparams->nexpr;
  KlAst** patterns = funcparams->exprs;
  klgen_stackalloc(gen, npattern);
  for (size_t i = 0; i < npattern; ++i) {
    KlAst* pattern = patterns[i];
    if (klast_kind(pattern) != KLAST_EXPR_ID) {
      if (i + 1 != npattern || !klgen_pattern_fastbinding(gen, pattern)) {
        /* is not last named parameter, or can not do fast deconstruction */
        /* move the to be deconstructed parameters to the top of stack */
        size_t nreserved = klgen_patterns_count_result(gen, patterns + i, npattern - i);
        klgen_emitmove(gen, i + nreserved, i, npattern - i, klgen_astposition(funcparams));
        klgen_stackalloc(gen, nreserved);
        kl_assert(klgen_stacktop(gen) == nreserved + npattern, "");
        size_t count = npattern;
        KlCStkId target = i + nreserved;
        while (count-- > i)
          target = klgen_pattern_binding(gen, patterns[count], target);
        kl_assert(target == i, "");
      }
      break;
    }
  }
  klgen_patterns_newsymbol(gen, patterns, npattern, 0);
  kl_assert(klgen_stacktop(gen) == klgen_patterns_count_result(gen, patterns, npattern), "");
}

static void klgen_exprfunc(KlGenUnit* gen, KlAstFunc* funcast, KlCStkId target) {
  KlCStkId stktop = klgen_stacktop(gen);
  KlGenUnit newgen;
  if (kl_unlikely(!klgen_init(&newgen, gen->symtblpool, gen->strings, gen->strtbl, gen, gen->config)))
    klgen_error_fatal(gen, "out of memory");
  if (setjmp(newgen.jmppos) == 0) {
    /* begin a new scope */
    klgen_pushsymtbl(&newgen);
    /* handle variable arguments */
    newgen.vararg = funcast->vararg;
    if (newgen.vararg)
      klgen_emit(&newgen, klinst_adjustargs(), klgen_astposition(funcast->params));
    /* deconstruct parameters */
    klgen_exprfunc_deconstruct_params(&newgen, funcast->params);

    /* generate code for function body */
    klgen_stmtlist(&newgen, funcast->block);
    /* add a return statement if 'return' is missing */
    if (!klast_mustreturn(funcast->block)) {
      if (newgen.symtbl->info.referenced)
        klgen_emit(&newgen, klinst_close(0), klgen_position(klast_end(funcast), klast_end(funcast)));
      klgen_emit(&newgen, klinst_return0(), klgen_position(klast_end(funcast), klast_end(funcast)));
    }
    /* close the scope */
    klgen_popsymtbl(&newgen);

    klgen_validate(&newgen);
    /* code generation is done */
    /* convert the 'newgen' to KlCode */
    KlCode* funccode = klgen_tocode_and_destroy(&newgen, funcast->params->nexpr);
    klgen_oomifnull(gen, funccode);
    /* add the new function to subfunction table of upper function */
    KlCIdx funcidx = klcodearr_size(&gen->subfunc);
    if (kl_unlikely(!klcodearr_push_back(&gen->subfunc, funccode))) {
      klcode_delete(funccode);
      klgen_error_fatal(gen, "out of memory");
    }
    klgen_emit(gen, funcast->is_method ? klinst_mkmethod(target, funcidx)
                                       : klinst_mkclosure(target, funcidx),
               klgen_astposition(funcast));
    if (target == stktop)
      klgen_stackalloc1(gen);
  } else {
    klgen_destroy(&newgen);
    if (target == stktop)
      klgen_stackalloc1(gen);
  }
}

static inline size_t abovelog2(size_t num) {
  size_t n = 0;
  while (((size_t)1 << n) < num)
    ++n;
  return n;
}

static void klgen_exprmap(KlGenUnit* gen, KlAstMap* mapast, KlCStkId target) {
  size_t sizefield = abovelog2(mapast->npair);
  if (sizefield < 3) sizefield = 3;
  size_t npair = mapast->npair;
  KlCStkId stktop = klgen_stackalloc1(gen);
  if (npair == 0) {
    klgen_emit(gen, klinst_mkmap(target, stktop, sizefield), klgen_astposition(mapast));
    if (target != stktop)
      klgen_stackfree(gen, stktop);
    return;
  }
  klgen_emit(gen, klinst_mkmap(stktop, stktop, sizefield), klgen_astposition(mapast));
  for (size_t i = 0; i < npair; ++i) {
    KlCStkId currstktop = klgen_stacktop(gen);
    KlAst* key = mapast->keys[i];
    KlAst* val = mapast->vals[i];
    KlCodeVal keypos = klgen_expr(gen, key);
    if (keypos.kind == KLVAL_INTEGER && klinst_inrange(keypos.intval, 8)) {
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putonstack(gen, &valpos, klgen_astposition(val));
      klgen_emit(gen, klinst_indexasi(valpos.index, target, keypos.intval), klgen_astposition(mapast));
    } else {
      klgen_putonstack(gen, &keypos, klgen_astposition(key));
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putonstack(gen, &valpos, klgen_astposition(val));
      klgen_emit(gen, klinst_indexas(valpos.index, target, keypos.index), klgen_astposition(mapast));
    }
    klgen_stackfree(gen, currstktop);
  }
  if (target != stktop) {
    klgen_emitmove(gen, target, stktop, 1, klgen_astposition(mapast));
    klgen_stackfree(gen, stktop);
  }
}

static void klgen_exprclasspost(KlGenUnit* gen, KlAstClass* classast, KlCStkId classpos) {
  size_t nfield = classast->nfield;
  KlCStkId stktop = klgen_stacktop(gen);
  for (size_t i = 0; i < nfield; ++i) {
    KlAstClassFieldDesc field = classast->fields[i];
    KlConstant constant = { .type = KLC_STRING, .string = field.name };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(gen, conent);
    if (field.shared) {
      KlCodeVal val = klgen_expr(gen, classast->vals[i]);
      klgen_putonstack(gen, &val, klgen_astposition(classast->vals[i]));
      if (klinst_inurange(conent->index, 8)) {
        klgen_emit(gen, klinst_setfieldc(val.index, classpos, conent->index), klgen_astposition(classast));
      } else {
        KlCStkId stkid = klgen_stackalloc1(gen);
        klgen_emit(gen, klinst_loadc(stkid, conent->index), klgen_astposition(classast));
        klgen_emit(gen, klinst_setfieldr(val.index, classpos, stkid), klgen_astposition(classast));
      }
    } else {
      klgen_emit(gen, klinst_newlocal(classpos, conent->index), klgen_astposition(classast));
    }
    klgen_stackfree(gen, stktop);
  }
}

static void klgen_exprclass(KlGenUnit* gen, KlAstClass* classast, KlCStkId target) {
  KlCStkId stktop = klgen_stacktop(gen);
  size_t class_size = abovelog2(classast->nfield);
  bool hasbase = classast->baseclass != NULL;
  if (hasbase) /* base is specified */
    klgen_exprtarget_noconst(gen, classast->baseclass, stktop);

  if (classast->nfield == 0) {
    klgen_emit(gen, klinst_mkclass(target, stktop, hasbase, class_size), klgen_astposition(classast));
    klgen_stackfree(gen, stktop != target ? stktop : target + 1);
  } else {
    klgen_emit(gen, klinst_mkclass(stktop, stktop, hasbase, class_size), klgen_astposition(classast));
    klgen_stackfree(gen, stktop + 1);
    klgen_exprclasspost(gen, classast, stktop);
    if (stktop != target)
      klgen_emitmove(gen, target, stktop, 1, klgen_astposition(classast));
    klgen_stackfree(gen, stktop != target ? stktop : target + 1);
  }
}

KlCodeVal klgen_exprconstant(KlGenUnit* gen, KlAstConstant* conast) {
  (void)gen;
  switch (conast->con.type) {
    case KLC_INT: {
      return klcodeval_integer(conast->con.intval);
    }
    case KLC_FLOAT: {
      return klcodeval_float(conast->con.floatval);
    }
    case KLC_BOOL: {
      return klcodeval_bool(conast->con.boolval);
    }
    case KLC_STRING: {
      return klcodeval_string(conast->con.string);
    }
    case KLC_NIL: {
      return klcodeval_nil();
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klgen_identifier(KlGenUnit* gen, KlAstIdentifier* idast) {
  KlSymbol* symbol = klgen_getsymbol(gen, idast->id);
  if (symbol) {
    return klcodeval_index(symbol->attr.kind, symbol->attr.idx);
  } else {  /* else is global variable */
    KlCStkId stkid = klgen_stackalloc1(gen);
    KlCIdx conidx = klgen_newstring(gen, idast->id);
    klgen_emit(gen, klinst_loadglobal(stkid, conidx), klgen_astposition(idast));
    return klcodeval_stack(stkid);
  }
}

static void klgen_method(KlGenUnit* gen, KlAst* objast, KlStrDesc method, KlAstExprList* args, KlFilePosition position, size_t nret, KlCStkId target) {
  KlCStkId base = klgen_stacktop(gen);
  klgen_exprtarget_noconst(gen, objast, base);
  size_t narg = klgen_passargs(gen, args);
  KlCIdx conidx = klgen_newstring(gen, method);
  klgen_emitmethod(gen, base, conidx, narg, nret, target, position);
  if (nret != KLINST_VARRES) {
    KlCStkId stktop = target + nret > base ? target + nret : base;
    klgen_stackfree(gen, stktop);
  }
}

static void klgen_exprcall(KlGenUnit* gen, KlAstCall* callast, size_t nret, KlCStkId target) {
  if (klast_kind(callast->callable) == KLAST_EXPR_DOT) {
    KlAstDot* dotast = klcast(KlAstDot*, callast->callable);
    klgen_method(gen, dotast->operand, dotast->field, callast->args, klgen_astposition(callast), nret, target);
    return;
  }
  KlCStkId base = klgen_stacktop(gen);
  klgen_exprtarget_noconst(gen, callast->callable, base);
  size_t narg = klgen_passargs(gen, callast->args);
  if (target == base) {
    klgen_emit(gen, klinst_scall(base, narg, nret), klgen_astposition(callast));
    if (nret != KLINST_VARRES)
      klgen_stackfree(gen, base + nret);
  } else {
    klgen_emitcall(gen, base, narg, nret, target, klgen_astposition(callast));
    if (nret != KLINST_VARRES)
      klgen_stackfree(gen, target + nret > base ? target + nret : base);
  }
}

void klgen_multival(KlGenUnit* gen, KlAst* ast, size_t nval, KlCStkId target) {
  kl_assert(nval != KLINST_VARRES, "");
  switch (klast_kind(ast)) {
    case KLAST_EXPR_YIELD: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlAstYield*, ast), nval);
      if (stktop != target && nval != 0) {
        klgen_emitmove(gen, target, stktop, nval, klgen_astposition(ast));
      }
      klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
      break;
    }
    case KLAST_EXPR_CALL: {
      klgen_exprcall(gen, klcast(KlAstCall*, ast), nval, target);
      break;
    }
    case KLAST_EXPR_WHERE: {
      klgen_exprwhere(gen, klcast(KlAstWhere*, ast), nval, target);
      break;
    }
    case KLAST_EXPR_VARARG: {
      klgen_exprvararg(gen, klcast(KlAstVararg*, ast), nval, target);
      break;
    }
    case KLAST_EXPR_LIST: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlAst** exprs = klcast(KlAstExprList*, ast)->exprs;
      size_t nexpr = klcast(KlAstExprList*, ast)->nexpr;
      if (nexpr == 0) {
        klgen_emitloadnils(gen, target, nval, klgen_astposition(ast));
        klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
        break;
      }
      for (size_t i = 0; i < nexpr - 1; ++i)
        klgen_multival(gen, exprs[i], 0, stktop);
      klgen_multival(gen, exprs[nexpr - 1], nval == 0 ? nval : 1, target);
      if (nval > 1) {
        klgen_emitloadnils(gen, target + 1, nval - 1, klgen_astposition(ast));
        klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
      }
      break;
    }
    case KLAST_EXPR_MATCH: {
      klgen_exprmatch(gen, klcast(KlAstMatch*, ast), target, nval != 0);
      if (nval > 1)
        klgen_emitloadnils(gen, target + 1, nval - 1, klgen_astposition(ast));
      break;
    }
    default: {
      KlCStkId stktop = klgen_stacktop(gen);
      if (nval == 0) {
        klgen_expr(gen, ast);
        klgen_stackfree(gen, stktop);
      } else {
        klgen_exprtarget_noconst(gen, ast, target);
        if (nval > 1)
          klgen_emitloadnils(gen, target + 1, nval - 1, klgen_astposition(ast));
      }
      klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
      break;
    }
  }
}

size_t klgen_trytakeall(KlGenUnit* gen, KlAst* ast, KlCodeVal* val) {
  switch (klast_kind(ast)) {
    case KLAST_EXPR_YIELD: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlAstYield*, ast), KLINST_VARRES);
      *val = klcodeval_stack(stktop);
      return KLINST_VARRES;
    }
    case KLAST_EXPR_CALL: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_exprcall(gen, klcast(KlAstCall*, ast), KLINST_VARRES, stktop);
      *val = klcodeval_stack(stktop);
      return KLINST_VARRES;
    }
    case KLAST_EXPR_WHERE: {
      KlCStkId stktop = klgen_stacktop(gen);
      size_t nres = klgen_exprwhere(gen, klcast(KlAstWhere*, ast), KLINST_VARRES, stktop);
      *val = klcodeval_stack(stktop);
      return nres;
    }
    case KLAST_EXPR_VARARG: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_exprvararg(gen, klcast(KlAstVararg*, ast), KLINST_VARRES, stktop);
      *val = klcodeval_stack(stktop);
      return KLINST_VARRES;
    }
    default: {
      *val = klgen_expr_onstack(gen, ast);
      return 1;
    }
  }
}

size_t klgen_takeall(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  switch (klast_kind(ast)) {
    case KLAST_EXPR_YIELD: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlAstYield*, ast), KLINST_VARRES);
      if (stktop != target)
        klgen_emitmove(gen, target, stktop, KLINST_VARRES, klgen_astposition(ast));
      return KLINST_VARRES;
    }
    case KLAST_EXPR_CALL: {
      klgen_exprcall(gen, klcast(KlAstCall*, ast), KLINST_VARRES, target);
      return KLINST_VARRES;
    }
    case KLAST_EXPR_WHERE: {
      return klgen_exprwhere(gen, klcast(KlAstWhere*, ast), KLINST_VARRES, target);
    }
    case KLAST_EXPR_VARARG: {
      klgen_exprvararg(gen, klcast(KlAstVararg*, ast), KLINST_VARRES, target);
      return KLINST_VARRES;
    }
    default: {
      klgen_exprtarget_noconst(gen, ast, target);
      return 1;
    }
  }
}

void klgen_exprlist_raw(KlGenUnit* gen, KlAst** asts, size_t nast, size_t nwanted, KlFilePosition filepos) {
  size_t nvalid = nwanted < nast ? nwanted : nast;
  if (nvalid == 0) {
    if (nwanted == 0) {
      KlCStkId stktop = klgen_stacktop(gen);
      for (size_t i = 0; i < nast; ++i)
        klgen_multival(gen, asts[i], 0, stktop);
    } else {  /* nast is 0 */
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_emitloadnils(gen, stktop, nwanted, filepos);
      klgen_stackalloc(gen, nwanted);
    }
    return;
  }
  size_t count = nvalid - 1;
  for (size_t i = 0; i < count; ++i)
    klgen_exprtarget_noconst(gen, asts[i], klgen_stacktop(gen));
  klgen_multival(gen, asts[count], nwanted - count, klgen_stacktop(gen));
  KlCStkId stktop = klgen_stacktop(gen);
  for (size_t i = nwanted; i < nast; ++i)
    klgen_multival(gen, asts[i], 0, stktop);
}

size_t klgen_passargs(KlGenUnit* gen, KlAstExprList* args) {
  if (args->nexpr == 0) return 0;
  /* evaluate the first args->nelem - 1 expressions */
  klgen_exprlist_raw(gen, args->exprs, args->nexpr - 1, args->nexpr - 1, klgen_astposition(args));
  KlAst* last = args->exprs[args->nexpr - 1];
  /* try to get all results of the last expression */
  size_t lastnres = klgen_takeall(gen, last, klgen_stacktop(gen));
  return lastnres == KLINST_VARRES ? lastnres : lastnres + args->nexpr - 1;
}

static KlCodeVal klgen_exprpre(KlGenUnit* gen, KlAstPre* preast, KlCStkId target) {
  switch (preast->op) {
    case KLTK_LEN: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, preast->operand);
      if (val.kind == KLVAL_STRING)
        return klcodeval_integer(val.string.length);
      KlFilePosition pos = klgen_astposition(preast);
      klgen_putonstack(gen, &val, pos);
      klgen_emit(gen, klinst_len(target, val.index), pos);
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_MINUS: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, preast->operand);
      if (val.kind == KLVAL_INTEGER) {
        return klcodeval_integer(-val.intval);
      } else if (val.kind == KLVAL_FLOAT) {
        return klcodeval_float(-val.floatval);
      }
      KlFilePosition pos = klgen_astposition(preast);
      klgen_putonstack(gen, &val, pos);
      klgen_emit(gen, klinst_neg(target, val.index), pos);
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_ASYNC: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, preast->operand);
      KlFilePosition pos = klgen_astposition(preast);
      klgen_putonstack(gen, &val, pos);
      klgen_emit(gen, klinst_async(target, val.index), pos);
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_NOT: {
      KlCodeVal res = klgen_exprboolval(gen, klast(preast), target);
      if (klcodeval_isconstant(res)) return res;
      return klcodeval_stack(target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlInstruction klgen_bininst(KlAstBin* binast, KlCStkId stkid, KlCStkId leftid, KlCStkId rightid) {
  switch (binast->op) {
    case KLTK_ADD:
      return klinst_add(stkid, leftid, rightid);
    case KLTK_MINUS:
      return klinst_sub(stkid, leftid, rightid);
    case KLTK_MUL:
      return klinst_mul(stkid, leftid, rightid);
    case KLTK_DIV:
      return klinst_div(stkid, leftid, rightid);
    case KLTK_MOD:
      return klinst_mod(stkid, leftid, rightid);
    case KLTK_IDIV:
      return klinst_idiv(stkid, leftid, rightid);
    case KLTK_CONCAT:
      return klinst_concat(stkid, leftid, rightid);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlInstruction klgen_bininsti(KlAstBin* binast, KlCStkId stkid, KlCStkId leftid, KlCInt imm) {
  switch (binast->op) {
    case KLTK_ADD:
      return klinst_addi(stkid, leftid, imm);
    case KLTK_MINUS:
      return klinst_subi(stkid, leftid, imm);
    case KLTK_MUL:
      return klinst_muli(stkid, leftid, imm);
    case KLTK_DIV:
      return klinst_divi(stkid, leftid, imm);
    case KLTK_MOD:
      return klinst_modi(stkid, leftid, imm);
    case KLTK_IDIV:
      return klinst_idivi(stkid, leftid, imm);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlInstruction klgen_bininstc(KlAstBin* binast, KlCStkId stkid, KlCStkId leftid, KlCIdx conidx) {
  switch (binast->op) {
    case KLTK_ADD:
      return klinst_addc(stkid, leftid, conidx);
    case KLTK_MINUS:
      return klinst_subc(stkid, leftid, conidx);
    case KLTK_MUL:
      return klinst_mulc(stkid, leftid, conidx);
    case KLTK_DIV:
      return klinst_divc(stkid, leftid, conidx);
    case KLTK_MOD:
      return klinst_modc(stkid, leftid, conidx);
    case KLTK_IDIV:
      return klinst_idivc(stkid, leftid, conidx);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlCodeVal klgen_tryarithcomptime(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCodeVal right, KlCStkId target) {
  if (left.kind == KLVAL_INTEGER && right.kind == KLVAL_INTEGER) {
    switch (binast->op) {
      case KLTK_ADD: {
        return klcodeval_integer(left.intval + right.intval);
      }
      case KLTK_MINUS: {
        return klcodeval_integer(left.intval - right.intval);
      }
      case KLTK_MUL: {
        return klcodeval_integer(left.intval * right.intval);
      }
      case KLTK_DIV: {
        KlCFloat f1 = (KlCFloat)left.intval;
        KlCFloat f2 = (KlCFloat)right.intval;
        return klcodeval_float(f1 / f2);
      }
      case KLTK_MOD: {
        if (right.intval == 0) {
          klgen_error(gen, klast_begin(binast), klast_end(binast), "divided by zero");
          return klcodeval_float(1.0);
        }
        return klcodeval_integer(left.intval % right.intval);
      }
      case KLTK_IDIV: {
        if (right.intval == 0) {
          klgen_error(gen, klast_begin(binast), klast_end(binast), "divided by zero");
          return klcodeval_float(1.0);
        }
        return klcodeval_integer(left.intval / right.intval);
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
  } else {
    KlCFloat l = left.kind == KLVAL_INTEGER ? (KlCFloat)left.intval : left.floatval;
    KlCFloat r = right.kind == KLVAL_INTEGER ? (KlCFloat)right.intval : right.floatval;
    switch (binast->op) {
      case KLTK_ADD: {
        return klcodeval_float(l + r);
      }
      case KLTK_MINUS: {
        return klcodeval_float(l - r);
      }
      case KLTK_MUL: {
        return klcodeval_float(l * r);
      }
      case KLTK_DIV: {
        return klcodeval_float(l / r);
      }
      case KLTK_MOD: {
        KlCStkId finstktop = klgen_stacktop(gen) == target ? target + 1 : klgen_stacktop(gen);
        klgen_putonstack(gen, &left, klgen_astposition(binast->loperand));
        KlCIdx conidx = klgen_newfloat(gen, r);
        if (klinst_inurange(conidx, 8)) {
          klgen_emit(gen, klinst_modc(target, left.index, conidx), klgen_astposition(binast));
        } else {
          KlCStkId tmp = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(binast->roperand));
          klgen_emit(gen, klinst_mod(target, left.index, tmp), klgen_astposition(binast));
        }
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      case KLTK_IDIV: {
        KlCStkId finstktop = klgen_stacktop(gen) == target ? target + 1 : klgen_stacktop(gen);
        klgen_putonstack(gen, &left, klgen_astposition(binast->loperand));
        KlCIdx conidx = klgen_newfloat(gen, r);
        if (klinst_inurange(conidx, 8)) {
          klgen_emit(gen, klinst_idivc(target, left.index, conidx), klgen_astposition(binast));
        } else {
          KlCStkId tmp = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(tmp, conidx), klgen_astposition(binast->roperand));
          klgen_emit(gen, klinst_idiv(target, left.index, tmp), klgen_astposition(binast));
        }
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      default: {
        kl_assert(false, "control flow should not reach here");
        return klcodeval_nil();
      }
    }
  }
}

static KlCodeVal klgen_exprbinleftliteral(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCStkId target) {
  kl_assert(klcodeval_isnumber(left) || left.kind == KLVAL_STRING, "");
  /* left is not on the stack, so the stack top is not changed */
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal right = klgen_expr(gen, binast->roperand);
  if ((klcodeval_isnumber(right) && klcodeval_isnumber(left)) ||
      (left.kind == KLVAL_STRING && right.kind == KLVAL_STRING)) {
    kl_assert(oristktop == klgen_stacktop(gen), "");
    /* try to compute at compile time */
    if (klcodeval_isnumber(left) && binast->op != KLTK_CONCAT) {
      return klgen_tryarithcomptime(gen, binast, left, right, target);
    }
    if (left.kind == KLVAL_STRING && binast->op == KLTK_CONCAT) {
      char* res = klstrtbl_concat(gen->strtbl, left.string, right.string);
      klgen_oomifnull(gen, res);
      KlStrDesc str = { .id = klstrtbl_stringid(gen->strtbl, res),
        .length = left.string.length + right.string.length };
      return klcodeval_string(str);
    }
    /* can not apply compile time computation, fall through */
  }
  klgen_putonstack(gen, &left, klgen_astposition(binast->loperand));
  if (right.kind == KLVAL_STACK) {
    KlInstruction inst = klgen_bininst(binast, target, left.index, right.index);
    klgen_emit(gen, inst, klgen_astposition(binast));
    klgen_stackfree(gen, oristktop == target ? target + 1 : oristktop);
    return klcodeval_stack(target);
  } else {
    return klgen_exprbinrightnonstk(gen, binast, left, right, target, oristktop);
  }
}

static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlAstBin* binast, KlCodeVal left, KlCodeVal right, KlCStkId target, KlCStkId oristktop) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  KlCStkId finstktop = oristktop == target ? target + 1 : oristktop;
  switch (right.kind) {
    case KLVAL_INTEGER: {
      if (binast->op == KLTK_CONCAT) {
        klgen_putonstack(gen, &right, klgen_astposition(binast->roperand));
        klgen_emit(gen, klgen_bininst(binast, target, left.index, right.index), klgen_astposition(binast));
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      /* first try OPI */
      if (klinst_inrange(right.intval, 8)) {
        klgen_emit(gen, klgen_bininsti(binast, target, left.index, right.intval), klgen_astposition(binast));
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      /* then try OPC */
      KlConEntry* conent = klgen_searchinteger(gen, right.intval);
      if (!conent && klinst_inurange(klcontbl_nextindex(gen->contbl), 8)) {
        conent = klcontbl_insert(gen->contbl, &(KlConstant) { .type = KLC_INT, .intval = right.intval });
        klgen_oomifnull(gen, conent);
      }
      if (conent && klinst_inurange(conent->index, 8)) {
        klgen_emit(gen, klgen_bininstc(binast, target, left.index, conent->index), klgen_astposition(binast));
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      /* then try LOADI, OP */
      if (klinst_inrange(right.intval, 16)) {
        KlCStkId stktop = klgen_stackalloc1(gen);
        klgen_emit(gen, klinst_loadi(stktop, right.intval), klgen_astposition(binast->roperand));
        klgen_emit(gen, klgen_bininst(binast, target, left.index, stktop), klgen_astposition(binast));
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      /* finally try LOADC, OP */
      KlCIdx conidx = conent ? conent->index : klgen_newinteger(gen, right.intval);
      KlCStkId stktop = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(stktop, conidx), klgen_astposition(binast->roperand));
      klgen_emit(gen, klgen_bininst(binast, target, left.index, stktop), klgen_astposition(binast));
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_FLOAT: {
      if (binast->op == KLTK_CONCAT) {
        klgen_putonstack(gen, &right, klgen_astposition(binast->roperand));
        klgen_emit(gen, klgen_bininst(binast, target, left.index, right.index), klgen_astposition(binast));
      } else {
        KlCIdx conidx = klgen_newfloat(gen, right.floatval);
        if (klinst_inurange(conidx, 8)) {
          klgen_emit(gen, klgen_bininstc(binast, target, left.index, conidx), klgen_astposition(binast));
        } else {
          KlCStkId stktop = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(stktop, conidx), klgen_astposition(binast->roperand));
          klgen_emit(gen, klgen_bininst(binast, target, left.index, stktop), klgen_astposition(binast));
        }
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_STRING: {
      KlCIdx conidx = klgen_newstring(gen, right.string);
      if (klinst_inurange(conidx, 8) && binast->op != KLTK_CONCAT) {
        klgen_emit(gen, klgen_bininstc(binast, target, left.index, conidx), klgen_astposition(binast));
      } else {
        KlCStkId stktop = klgen_stackalloc1(gen);
        klgen_emit(gen, klinst_loadc(stktop, conidx), klgen_astposition(binast->roperand));
        klgen_emit(gen, klgen_bininst(binast, target, left.index, stktop), klgen_astposition(binast));
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_REF:
    case KLVAL_BOOL:
    case KLVAL_NIL: {
      klgen_putonstack(gen, &right, klgen_astposition(binast->roperand));
      klgen_emit(gen, klgen_bininst(binast, target, left.index, right.index), klgen_astposition(binast));
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klgen_exprbin(KlGenUnit* gen, KlAstBin* binast, KlCStkId target) {
  if (kltoken_isarith(binast->op) || binast->op == KLTK_CONCAT) {
    KlCStkId leftid = klgen_stacktop(gen);
    KlCodeVal left = klgen_expr(gen, binast->loperand);
    if (klcodeval_isnumber(left) || left.kind == KLVAL_STRING) {
      return klgen_exprbinleftliteral(gen, binast, left, target);
    } else {
      klgen_putonstack(gen, &left, klgen_astposition(binast->loperand));
    }
    /* now left is on stack */
    KlCodeVal right = klgen_expr(gen, binast->roperand);
    if (right.kind != KLVAL_STACK)
      return klgen_exprbinrightnonstk(gen, binast, left, right, target, leftid);
    /* now both are on stack */
    KlInstruction inst = klgen_bininst(binast, target, left.index, right.index);
    klgen_emit(gen, inst, klgen_astposition(binast));
    klgen_stackfree(gen, leftid == target ? target + 1 : leftid);
    return klcodeval_stack(leftid);
  } else {  /* else is boolean expression */
    KlCodeVal res = klgen_exprboolval(gen, klast(binast), target);
    if (klcodeval_isconstant(res)) return res;
    return klcodeval_stack(target);
  }
}

static KlCodeVal klgen_exprappend(KlGenUnit* gen, KlAstPost* postast) {
  kl_assert(postast->op == KLTK_APPEND, "");
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal appendable = klgen_expr(gen, postast->operand);
  klgen_putonstack(gen, &appendable, klgen_astposition(postast->operand));
  KlAstExprList* exprlist = klcast(KlAstExprList*, postast->post);
  if (exprlist->nexpr == 1 && klast_kind(exprlist->exprs[0]) == KLAST_EXPR_ID) {
    KlCodeVal val = klgen_expr(gen, exprlist->exprs[0]);
    klgen_emit(gen, klinst_append(appendable.index, val.index, 1), klgen_astposition(postast));
  } else {
    KlCStkId base = klgen_stacktop(gen);
    size_t narg = klgen_passargs(gen, exprlist);
    klgen_emit(gen, klinst_append(appendable.index, base, narg), klgen_astposition(postast));
  }
  klgen_stackfree(gen, stktop);
  return appendable;
}

static KlCodeVal klgen_exprpost(KlGenUnit* gen, KlAstPost* postast, KlCStkId target, bool append_target) {
  switch (postast->op) {
    case KLTK_INDEX: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal indexable = klgen_expr(gen, postast->operand);
      klgen_putonstack(gen, &indexable, klgen_astposition(postast->operand));
      KlCodeVal index = klgen_expr(gen, postast->post);
      if (index.kind == KLVAL_INTEGER && klinst_inrange(index.intval, 8)) {
        klgen_emit(gen, klinst_indexi(target, indexable.index, index.intval), klgen_astposition(postast));
      } else {
        klgen_putonstack(gen, &index, klgen_astposition(postast->post));
        klgen_emit(gen, klinst_index(target, indexable.index, index.index), klgen_astposition(postast));
      }
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_APPEND: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlCodeVal appendable = klgen_exprappend(gen, postast);
      if (append_target && target != appendable.index)
        klgen_emitmove(gen, target, appendable.index, 1, klgen_astposition(postast));
      klgen_stackfree(gen, target == stktop ? target + 1 : stktop);
      return append_target ? klcodeval_stack(target) : appendable;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static void klgen_exprnew(KlGenUnit* gen, KlAstNew* newast, KlCStkId target) {
  KlCStkId oristktop = klgen_stacktop(gen);
  KlCodeVal klclass = klgen_expr(gen, newast->klclass);
  klgen_putonstack(gen, &klclass, klgen_astposition(newast->klclass));
  klgen_emit(gen, klinst_newobj(target, klclass.index), klgen_astposition(newast));
  if (newast->args) {
    if (target == klgen_stacktop(gen))
      klgen_stackalloc1(gen);
    KlCStkId stktop = klgen_stackalloc1(gen);
    klgen_emitmove(gen, stktop, target, 1, klgen_astposition(newast));
    size_t narg = klgen_passargs(gen, newast->args);
    KlCIdx conidx = klgen_newstring(gen, gen->strings->constructor);
    klgen_emitmethod(gen, stktop, conidx, narg, 0, stktop, klgen_astposition(newast));
  }
  klgen_stackfree(gen, oristktop == target ? target + 1 : oristktop);
}

static void klgen_exprdot(KlGenUnit* gen, KlAstDot* dotast, KlCStkId target) {
  KlCStkId stktop = klgen_stacktop(gen);
  KlCodeVal obj = klgen_expr(gen, dotast->operand);
  KlCStkId conidx = klgen_newstring(gen, dotast->field);
  if (obj.kind == KLVAL_REF) {
    if (klinst_inurange(conidx, 8)) {
      klgen_emit(gen, klinst_refgetfieldc(target, obj.index, conidx), klgen_astposition(dotast));
    } else {
      KlCStkId stkid = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(stkid, conidx), klgen_astposition(dotast));
      klgen_emit(gen, klinst_refgetfieldr(target, obj.index, stkid), klgen_astposition(dotast));
    }
  } else {
    klgen_putonstack(gen, &obj, klgen_astposition(dotast->operand));
    if (klinst_inurange(conidx, 8)) {
      klgen_emit(gen, klinst_getfieldc(target, obj.index, conidx), klgen_astposition(dotast));
    } else {
      KlCStkId stkid = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(stkid, conidx), klgen_astposition(dotast));
      klgen_emit(gen, klinst_getfieldr(target, obj.index, stkid), klgen_astposition(dotast));
    }
  }
  klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
}


KlCodeVal klgen_expr(KlGenUnit* gen, KlAst* ast) {
  switch (klast_kind(ast)) {
    case KLAST_EXPR_ARR: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprarr(gen, klcast(KlAstArray*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_ARRGEN: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprarrgen(gen, klcast(KlAstArrayGenerator*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_MATCH: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprmatch(gen, klcast(KlAstMatch*, ast), target, true);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_WHERE: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprwhere(gen, klcast(KlAstWhere*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_MAP: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprmap(gen, klcast(KlAstMap*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_CLASS: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprclass(gen, klcast(KlAstClass*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_CONSTANT: {
      return klgen_exprconstant(gen, klcast(KlAstConstant*, ast));
    }
    case KLAST_EXPR_ID: {
      return klgen_identifier(gen, klcast(KlAstIdentifier*, ast));
    }
    case KLAST_EXPR_VARARG: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprvararg(gen, klcast(KlAstVararg*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_LIST: {
      return klgen_exprlist(gen, klcast(KlAstExprList*, ast));
    }
    case KLAST_EXPR_PRE: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprpre(gen, klcast(KlAstPre*, ast), target);
    }
    case KLAST_EXPR_NEW: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprnew(gen, klcast(KlAstNew*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_YIELD: {
      KlCStkId stkid = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlAstYield*, ast), 1);
      return klcodeval_stack(stkid);
    }
    case KLAST_EXPR_POST: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprpost(gen, klcast(KlAstPost*, ast), target, false);
    }
    case KLAST_EXPR_CALL: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprcall(gen, klcast(KlAstCall*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_DOT: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprdot(gen, klcast(KlAstDot*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_FUNC: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprfunc(gen, klcast(KlAstFunc*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_BIN: {
      KlCStkId target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprbin(gen, klcast(KlAstBin*, ast), target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlAst* ast, KlCStkId target) {
  switch (klast_kind(ast)) {
    case KLAST_EXPR_ARR: {
      klgen_exprarr(gen, klcast(KlAstArray*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_ARRGEN: {
      klgen_exprarrgen(gen, klcast(KlAstArrayGenerator*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_MATCH: {
      klgen_exprmatch(gen, klcast(KlAstMatch*, ast), target, true);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_WHERE: {
      klgen_exprwhere(gen, klcast(KlAstWhere*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_MAP: {
      klgen_exprmap(gen, klcast(KlAstMap*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_CLASS: {
      klgen_exprclass(gen, klcast(KlAstClass*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_CONSTANT: {
      return klgen_exprconstant(gen, klcast(KlAstConstant*, ast));
    }
    case KLAST_EXPR_ID: {
      KlCStkId stktop = klgen_stacktop(gen);
      KlAstIdentifier* idast = klcast(KlAstIdentifier*, ast);
      KlSymbol* symbol = klgen_getsymbol(gen, idast->id);
      if (symbol) {
        KlCodeVal idval = klcodeval_index(symbol->attr.kind, symbol->attr.idx);
        klgen_loadval(gen, target, idval, klgen_astposition(ast));
      } else {  /* else is global variable */
        KlCIdx conidx = klgen_newstring(gen, idast->id);
        klgen_emit(gen, klinst_loadglobal(target, conidx), klgen_astposition(idast));
      }
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_VARARG: {
      klgen_exprvararg(gen, klcast(KlAstVararg*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_LIST: {
      return klgen_exprlist_target(gen, klcast(KlAstExprList*, ast), target);
    }
    case KLAST_EXPR_PRE: {
      return klgen_exprpre(gen, klcast(KlAstPre*, ast), target);
    }
    case KLAST_EXPR_NEW: {
      klgen_exprnew(gen, klcast(KlAstNew*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_YIELD: {
      KlCStkId stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlAstYield*, ast), 1);
      if (target != stktop)
        klgen_emitmove(gen, target, stktop, 1, klgen_astposition(ast));
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_POST: {
      return klgen_exprpost(gen, klcast(KlAstPost*, ast), target, true);
    }
    case KLAST_EXPR_CALL: {
      klgen_exprcall(gen, klcast(KlAstCall*, ast), 1, target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_DOT: {
      klgen_exprdot(gen, klcast(KlAstDot*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_FUNC: {
      klgen_exprfunc(gen, klcast(KlAstFunc*, ast), target);
      return klcodeval_stack(target);
    }
    case KLAST_EXPR_BIN: {
      return klgen_exprbin(gen, klcast(KlAstBin*, ast), target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}
