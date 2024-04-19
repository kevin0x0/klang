#include "klang/include/code/klgen.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/code/klgen_pattern.h"
#include "klang/include/code/klgen_exprbool.h"
#include "klang/include/code/klgen_stmt.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klstrtab.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klinst.h"
#include <setjmp.h>

static KlCodeVal klgen_exprbinleftliteral(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, size_t target);
static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target, size_t oristktop);

void klgen_exprarr(KlGenUnit* gen, KlCstArray* arrcst, size_t target) {
  size_t argbase = klgen_stacktop(gen);
  size_t nval = klgen_passargs(gen, arrcst->vals);
  klgen_emit(gen, klinst_mkarray(target, argbase, nval), klgen_cstposition(arrcst));
}

void klgen_exprarrgen(KlGenUnit* gen, KlCstArrayGenerator* arrgencst, size_t target) {
  size_t stktop = klgen_stackalloc1(gen);
  klgen_newsymbol(gen, arrgencst->arrid, stktop, klgen_position(klcst_begin(arrgencst), klcst_begin(arrgencst)));
  bool needclose = klgen_stmtblockpure(gen, klcast(KlCstStmtList*, arrgencst->block));
  if (needclose)
    klgen_emit(gen, klinst_close(stktop), klgen_cstposition(arrgencst));
  if (target != stktop) {
    klgen_emitmove(gen, target, stktop, 1, klgen_cstposition(arrgencst));
    klgen_stackfree(gen, stktop);
  }
}

static inline size_t klgen_takeval(KlGenUnit* gen, KlCst* cst, size_t nwanted, size_t target) {
  if (nwanted == KLINST_VARRES) {
    return klgen_takeall(gen, cst, target);
  } else {
    klgen_multival(gen, cst, nwanted, target);
    return nwanted;
  }
}

static size_t klgen_exprwhere(KlGenUnit* gen, KlCstWhere* wherecst, size_t nwanted, size_t target) {
  size_t oristktop = klgen_stacktop(gen);
  klgen_pushsymtbl(gen);
  klgen_stmtlistpure(gen, klcast(KlCstStmtList*, wherecst->block));
  if (gen->symtbl->info.referenced) {
    /* some symbol in this 'where scope' is referenced by some sub-function,
     * so we evaluate expression to the top of stack to avoid destroying
     * referenced value before the scope closed. */
    size_t stktop = klgen_stacktop(gen);
    size_t nres = klgen_takeval(gen, wherecst->expr, nwanted, stktop);
    /* evaluation is done, close */
    klgen_emit(gen, klinst_close(oristktop), klgen_cstposition(wherecst));
    klgen_popsymtbl(gen);
    /* move results to 'target' */
    klgen_emitmove(gen, target, stktop, nres, klgen_cstposition(wherecst->expr));
    if (nres != KLINST_VARRES)
      klgen_stackfree(gen, oristktop > target + nres ? oristktop : target + nres);
    return nres;
  } else {
    /* no symbol is referenced, so we evaluate the expression directly to its 'target' */
    size_t nres = klgen_takeval(gen, wherecst->expr, nwanted, target);
    /* evaluation is complete */
    klgen_popsymtbl(gen);
    if (nres != KLINST_VARRES)
      klgen_stackfree(gen, oristktop > target + nres ? oristktop : target + nres);
    return nres;
  }
}

/* deconstruct parameters */
static void klgen_exprfunc_deconstruct_params(KlGenUnit* gen, KlCstTuple* funcparams) {
  kl_assert(klgen_stacktop(gen) == 0, "");
  size_t npattern = funcparams->nelem;
  KlCst** patterns = funcparams->elems;
  klgen_stackalloc(gen, npattern);
  for (size_t i = 0; i < npattern; ++i) {
    KlCst* pattern = klgen_exprpromotion(patterns[i]);
    if (klcst_kind(pattern) != KLCST_EXPR_ID) {
      if (i + 1 != npattern || !klgen_pattern_fastdeconstruct(gen, pattern)) {
        /* is not last named parameter, or can not do fast deconstruction */
        /* move the to be deconstructed parameters to the top of stack */
        size_t nreserved = klgen_patterns_count_result(gen, patterns + i, npattern - i);
        klgen_emitmove(gen, i + nreserved, i, npattern - i, klgen_cstposition(funcparams));
        klgen_stackalloc(gen, nreserved);
        kl_assert(klgen_stacktop(gen) == nreserved + npattern, "");
        size_t count = npattern;
        size_t target = i + nreserved;
        while (count-- > i)
          target -= klgen_pattern_deconstruct(gen, patterns[count], target);
        kl_assert(target == i, "");
      }
    }
  }
  klgen_patterns_newsymbol(gen, patterns, npattern, 0);
  kl_assert(klgen_stacktop(gen) == klgen_patterns_count_result(gen, patterns, npattern), "");
}

void klgen_exprfunc(KlGenUnit* gen, KlCstFunc* funccst, size_t target) {
  size_t stktop = klgen_stacktop(gen);
  KlGenUnit newgen;
  if (kl_unlikely(!klgen_init(&newgen, gen->symtblpool, gen->strtab, gen, gen->input, gen->klerror)))
    klgen_error_fatal(gen, "out of memory");
  if (setjmp(newgen.jmppos) == 0) {
    /* the scope is already created in klgen_init() */
    /* handle variable arguments */
    newgen.vararg = funccst->vararg;
    if (newgen.vararg)
      klgen_emit(&newgen, klinst_adjustargs(), klgen_cstposition(funccst->params));
    /* deconstruct parameters */
    klgen_exprfunc_deconstruct_params(&newgen, klcast(KlCstTuple*, funccst->params));

    /* generate code for function body */
    klgen_stmtlist(&newgen, klcast(KlCstStmtList*, funccst->block));
    /* add a return statement if 'return' is missing */
    if (!klcst_mustreturn(klcast(KlCstStmtList*, funccst->block)))
      klgen_emit(&newgen, klinst_return0(), klgen_position(klcst_end(funccst), klcst_end(funccst)));
    klgen_validate(&newgen);
    /* code generation is done */
    /* convert the 'newgen' to KlCode */
    KlCode* funccode = klgen_tocode_and_destroy(&newgen, klcast(KlCstTuple*, funccst->params)->nelem);
    klgen_oomifnull(gen, funccode);
    /* add the new function to subfunction table of upper function */
    size_t funcidx = klcodearr_size(&gen->subfunc);
    if (kl_unlikely(!klcodearr_push_back(&gen->subfunc, funccode))) {
      klcode_delete(funccode);
      klgen_error_fatal(gen, "out of memory");
    }
    klgen_emit(gen, funccst->is_method ? klinst_mkmethod(target, funcidx)
                                       : klinst_mkclosure(target, funcidx),
               klgen_cstposition(funccst));
    if (target == stktop)
      klgen_stackalloc1(gen);
  } else {
  }
}

static inline size_t abovelog2(size_t num) {
  size_t n = 0;
  while ((1 << n) < num)
    ++n;
  return n;
}

void klgen_exprmap(KlGenUnit* gen, KlCstMap* mapcst, size_t target) {
  size_t sizefield = abovelog2(mapcst->npair);
  if (sizefield < 3) sizefield = 3;
  size_t npair = mapcst->npair;
  size_t stktop = klgen_stackalloc1(gen);
  if (npair == 0) {
    klgen_emit(gen, klinst_mkmap(target, stktop, sizefield), klgen_cstposition(mapcst));
    if (target != stktop)
      klgen_stackfree(gen, stktop);
    return;
  }
  klgen_emit(gen, klinst_mkmap(stktop, stktop, sizefield), klgen_cstposition(mapcst));
  for (size_t i = 0; i < npair; ++i) {
    size_t currstktop = klgen_stacktop(gen);
    KlCst* key = mapcst->keys[i];
    KlCst* val = mapcst->vals[i];
    KlCodeVal keypos = klgen_expr(gen, key);
    if (keypos.kind == KLVAL_INTEGER && klinst_inrange(keypos.intval, 8)) {
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putonstack(gen, &valpos, klgen_cstposition(val));
      klgen_emit(gen, klinst_indexasi(valpos.index, target, keypos.intval), klgen_cstposition(mapcst));
    } else {
      klgen_putonstack(gen, &keypos, klgen_cstposition(key));
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putonstack(gen, &valpos, klgen_cstposition(val));
      klgen_emit(gen, klinst_indexas(valpos.index, target, keypos.index), klgen_cstposition(mapcst));
    }
    klgen_stackfree(gen, currstktop);
  }
  if (target != stktop) {
    klgen_emitmove(gen, target, stktop, 1, klgen_cstposition(mapcst));
    klgen_stackfree(gen, stktop);
  }
}

static void klgen_exprclasspost(KlGenUnit* gen, KlCstClass* classcst, size_t classpos) {
  size_t nfield = classcst->nfield;
  size_t stktop = klgen_stacktop(gen);
  for (size_t i = 0; i < nfield; ++i) {
    KlCstClassFieldDesc field = classcst->fields[i];
    KlConstant constant = { .type = KL_STRING, .string = field.name };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(gen, conent);
    if (field.shared) {
      KlCodeVal val = klgen_expr(gen, classcst->vals[i]);
      klgen_putonstack(gen, &val, klgen_cstposition(classcst->vals[i]));
      if (klinst_inurange(conent->index, 8)) {
        klgen_emit(gen, klinst_setfieldc(val.index, classpos, conent->index), klgen_cstposition(classcst));
      } else {
        size_t stkid = klgen_stackalloc1(gen);
        klgen_emit(gen, klinst_loadc(stkid, conent->index), klgen_cstposition(classcst));
        klgen_emit(gen, klinst_setfieldr(val.index, classpos, stkid), klgen_cstposition(classcst));
      }
    } else {
      klgen_emit(gen, klinst_newlocal(classpos, conent->index), klgen_cstposition(classcst));
    }
    klgen_stackfree(gen, stktop);
  }
}

void klgen_exprclass(KlGenUnit* gen, KlCstClass* classcst, size_t target) {
  size_t stktop = klgen_stacktop(gen);
  size_t class_size = abovelog2(classcst->nfield);
  bool hasbase = classcst->baseclass != NULL;
  if (hasbase) {
    /* base is specified */
    KlCodeVal base = klgen_exprtarget(gen, classcst->baseclass, stktop);
    if (klcodeval_isconstant(base))
      klgen_putonstktop(gen, &base, klgen_cstposition(classcst->baseclass));
    kl_assert(base.index == stktop, "");
  }
  if (classcst->nfield == 0) {
    klgen_emit(gen, klinst_mkclass(target, stktop, hasbase, class_size), klgen_cstposition(classcst));
  } else {
    klgen_emit(gen, klinst_mkclass(stktop, stktop, hasbase, class_size), klgen_cstposition(classcst));
    klgen_exprclasspost(gen, classcst, stktop);
  }
  if (stktop != target)
    klgen_emitmove(gen, target, stktop, 1, klgen_cstposition(classcst));
  klgen_stackfree(gen, stktop != target ? stktop : target + 1);
}

static KlCodeVal klgen_constant(KlGenUnit* gen, KlCstConstant* concst) {
  (void)gen;
  switch (concst->con.type) {
    case KL_INT: {
      return klcodeval_integer(concst->con.intval);
    }
    case KL_FLOAT: {
      return klcodeval_float(concst->con.floatval);
    }
    case KL_BOOL: {
      return klcodeval_bool(concst->con.boolval);
    }
    case KL_STRING: {
      return klcodeval_string(concst->con.string);
    }
    case KL_NIL: {
      return klcodeval_nil();
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klgen_identifier(KlGenUnit* gen, KlCstIdentifier* idcst) {
  KlSymbol* symbol = klgen_getsymbol(gen, idcst->id);
  if (symbol) {
    return klcodeval_index(symbol->attr.kind, symbol->attr.idx);
  } else {  /* else is global variable */
    size_t stkid = klgen_stackalloc1(gen);
    KlConstant con = { .type = KL_STRING, .string = idcst->id };
    KlConEntry* conent = klcontbl_get(gen->contbl, &con);
    klgen_oomifnull(gen, conent);
    klgen_emit(gen, klinst_loadglobal(stkid, conent->index), klgen_cstposition(idcst));
    return klcodeval_stack(stkid);
  }
}

void klgen_method(KlGenUnit* gen, KlCst* objcst, KlStrDesc method, KlCst* args, KlFilePosition position, size_t nret, size_t target) {
  size_t base = klgen_stacktop(gen);
  klgen_exprtarget_noconst(gen, objcst, base);
  size_t narg = klgen_passargs(gen, args);
  KlConstant con = { .type = KL_STRING, .string = method };
  KlConEntry* conent = klcontbl_get(gen->contbl, &con);
  klgen_oomifnull(gen, conent);
  klgen_emitmethod(gen, base, conent->index, narg, nret, target, position);
  if (nret != KLINST_VARRES) {
    size_t stktop = target + nret > base ? target + nret : base;
    klgen_stackfree(gen, stktop);
  }
}

void klgen_call(KlGenUnit* gen, KlCstPost* callcst, size_t nret, size_t target) {
  if (klcst_kind(callcst->operand) == KLCST_EXPR_DOT) {
    KlCstDot* dotcst = klcast(KlCstDot*, callcst->operand);
    klgen_method(gen, dotcst->operand, dotcst->field, callcst->post, klgen_cstposition(callcst), nret, target);
    return;
  }
  size_t base = klgen_stacktop(gen);
  klgen_exprtarget_noconst(gen, callcst->operand, base);
  size_t narg = klgen_passargs(gen, callcst->post);
  klgen_emit(gen, klinst_call(base, narg, nret), klgen_cstposition(callcst));
  if (nret != KLINST_VARRES)
    klgen_stackfree(gen, base + nret);
  if (nret == 0 || target == base) return;
  klgen_emitmove(gen, target, base, nret, klgen_cstposition(callcst));
  if (nret != KLINST_VARRES)
    klgen_stackfree(gen, target + nret > base ? target + nret : base);
}

void klgen_multival(KlGenUnit* gen, KlCst* cst, size_t nval, size_t target) {
  kl_assert(nval != KLINST_VARRES, "");
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_YIELD: {
      size_t stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), nval);
      if (stktop != target && nval != 0) {
        klgen_emitmove(gen, target, stktop, nval, klgen_cstposition(cst));
      }
      klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
      break;
    }
    case KLCST_EXPR_CALL: {
      klgen_call(gen, klcast(KlCstPost*, cst), nval, target);
      break;
    }
    case KLCST_EXPR_WHERE: {
      klgen_exprwhere(gen, klcast(KlCstWhere*, cst), nval, target);
      break;
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    default: {
      size_t stktop = klgen_stacktop(gen);
      klgen_exprtarget_noconst(gen, cst, target);
      if (nval > 1)
        klgen_emitloadnils(gen, target + 1, nval - 1, klgen_cstposition(cst));
      klgen_stackfree(gen, target + nval > stktop ? target + nval : stktop);
      break;
    }
  }
}

size_t klgen_trytakeall(KlGenUnit* gen, KlCst* cst, KlCodeVal* val) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_YIELD: {
      size_t stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), KLINST_VARRES);
      *val = klcodeval_stack(stktop);
      return KLINST_VARRES;
    }
    case KLCST_EXPR_CALL: {
      size_t stktop = klgen_stacktop(gen);
      klgen_call(gen, klcast(KlCstPost*, cst), KLINST_VARRES, stktop);
      *val = klcodeval_stack(stktop);
      return KLINST_VARRES;
    }
    case KLCST_EXPR_WHERE: {
      size_t stktop = klgen_stacktop(gen);
      size_t nres = klgen_exprwhere(gen, klcast(KlCstWhere*, cst), KLINST_VARRES, stktop);
      *val = klcodeval_stack(stktop);
      return nres;
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    default: {
      *val = klgen_expr_onstack(gen, cst);
      return 1;
    }
  }
}

size_t klgen_takeall(KlGenUnit* gen, KlCst* cst, size_t target) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_YIELD: {
      size_t stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), KLINST_VARRES);
      if (stktop != target)
        klgen_emitmove(gen, target, stktop, KLINST_VARRES, klgen_cstposition(cst));
      return KLINST_VARRES;
    }
    case KLCST_EXPR_CALL: {
      klgen_call(gen, klcast(KlCstPost*, cst), KLINST_VARRES, target);
      return KLINST_VARRES;
    }
    case KLCST_EXPR_WHERE: {
      return klgen_exprwhere(gen, klcast(KlCstWhere*, cst), KLINST_VARRES, target);
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    default: {
      klgen_exprtarget_noconst(gen, cst, target);
      return 1;
    }
  }
}

void klgen_exprlist_raw(KlGenUnit* gen, KlCst** csts, size_t ncst, size_t nwanted, KlFilePosition filepos) {
  size_t nvalid = nwanted < ncst ? nwanted : ncst;
  if (nvalid == 0) {
    if (nwanted == 0) {
      size_t stktop = klgen_stacktop(gen);
      for (size_t i = 0; i < ncst; ++i) {
        klgen_expr(gen, csts[i]);
        klgen_stackfree(gen, stktop);
      }
    } else {  /* ncst is 0 */
      size_t stktop = klgen_stacktop(gen);
      klgen_emitloadnils(gen, stktop, nwanted, filepos);
      klgen_stackalloc(gen, nwanted);
    }
    return;
  }
  size_t count = nvalid - 1;
  for (size_t i = 0; i < count; ++i)
    klgen_exprtarget_noconst(gen, csts[i], klgen_stacktop(gen));
  klgen_multival(gen, csts[count], nwanted - count, klgen_stacktop(gen));
  for (size_t i = nwanted; i < ncst; ++i) {
    size_t stktop = klgen_stacktop(gen);
    klgen_expr(gen, csts[i]);
    klgen_stackfree(gen, stktop);
  }
}

size_t klgen_passargs(KlGenUnit* gen, KlCst* args) {
  kl_assert(klcst_kind(args) == KLCST_EXPR_TUPLE, "");
  KlCstTuple* tuple = klcast(KlCstTuple*, args);
  if (tuple->nelem == 0) return 0;
  /* evaluate the first tuple->nelem - 1 expressions */
  klgen_exprlist_raw(gen, tuple->elems, tuple->nelem - 1, tuple->nelem - 1, klgen_cstposition(args));
  KlCst* last = tuple->elems[tuple->nelem - 1];
  /* try to get all results of the last expression */
  return klgen_takeall(gen, last, klgen_stacktop(gen)) + tuple->nelem - 1;
}

KlCodeVal klgen_exprpre(KlGenUnit* gen, KlCstPre* precst, size_t target) {
  switch (precst->op) {
    case KLTK_MINUS: {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, precst->operand);
      if (val.kind == KLVAL_INTEGER) {
        return klcodeval_integer(-val.intval);
      } else if (val.kind == KLVAL_FLOAT) {
        return klcodeval_float(-val.floatval);
      }
      KlFilePosition pos = klgen_cstposition(precst);
      klgen_putonstack(gen, &val, pos);
      klgen_emit(gen, klinst_neg(target, val.index), pos);
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_ASYNC: {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, precst->operand);
      KlFilePosition pos = klgen_cstposition(precst);
      klgen_putonstack(gen, &val, pos);
      klgen_emit(gen, klinst_async(target, val.index), pos);
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_NOT: {
      KlCodeVal res = klgen_exprboolval(gen, klcst(precst), target);
      if (klcodeval_isconstant(res)) return res;
      return klcodeval_stack(target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlInstruction klgen_bininst(KlCstBin* bincst, size_t stkid, size_t leftid, size_t rightid) {
  switch (bincst->op) {
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

static KlInstruction klgen_bininsti(KlCstBin* bincst, size_t stkid, size_t leftid, KlInt imm) {
  switch (bincst->op) {
    case KLTK_ADD:
      return klinst_addi(stkid, leftid, imm);
    case KLTK_MINUS:
      return klinst_subi(stkid, leftid, imm);
    case KLTK_MUL:
      return klinst_muli(stkid, leftid, imm);
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

static KlInstruction klgen_bininstc(KlCstBin* bincst, size_t stkid, size_t leftid, size_t conidx) {
  switch (bincst->op) {
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

static KlCodeVal klgen_tryarithcomptime(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target) {
  if (left.kind == KLVAL_INTEGER && right.kind == KLVAL_INTEGER) {
    switch (bincst->op) {
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
        KlFloat f1 = (KlFloat)left.intval;
        KlFloat f2 = (KlFloat)right.intval;
        return klcodeval_float(f1 / f2);
      }
      case KLTK_MOD: {
        if (right.intval == 0) {
          klgen_error(gen, klcst_begin(bincst), klcst_end(bincst), "divided by zero");
          return klcodeval_float(1.0);
        }
        return klcodeval_integer(left.intval % right.intval);
      }
      case KLTK_IDIV: {
        if (right.intval == 0) {
          klgen_error(gen, klcst_begin(bincst), klcst_end(bincst), "divided by zero");
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
    KlFloat l = left.kind == KLVAL_INTEGER ? (KlFloat)left.intval : left.floatval;
    KlFloat r = right.kind == KLVAL_INTEGER ? (KlFloat)right.intval : right.floatval;
    switch (bincst->op) {
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
        size_t finstktop = klgen_stacktop(gen) == target ? target + 1 : klgen_stacktop(gen);
        klgen_putonstack(gen, &left, klgen_cstposition(bincst->loperand));
        KlConstant constant = { .type = KL_FLOAT, .floatval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
        klgen_oomifnull(gen, conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_emit(gen, klinst_modc(target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t tmp = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(bincst->roperand));
          klgen_emit(gen, klinst_mod(target, left.index, tmp), klgen_cstposition(bincst));
        }
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      case KLTK_IDIV: {
        size_t finstktop = klgen_stacktop(gen) == target ? target + 1 : klgen_stacktop(gen);
        klgen_putonstack(gen, &left, klgen_cstposition(bincst->loperand));
        KlConstant constant = { .type = KL_FLOAT, .floatval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
        klgen_oomifnull(gen, conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_emit(gen, klinst_idivc(target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t tmp = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(bincst->roperand));
          klgen_emit(gen, klinst_idiv(target, left.index, tmp), klgen_cstposition(bincst));
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

static KlCodeVal klgen_exprbinleftliteral(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, size_t target) {
  kl_assert(klcodeval_isnumber(left) || left.kind == KLVAL_STRING, "");
  /* left is not on the stack, so the stack top is not changed */
  size_t oristktop = klgen_stacktop(gen);
  KlCodeVal right = klgen_expr(gen, bincst->roperand);
  if ((klcodeval_isnumber(right) && klcodeval_isnumber(left)) ||
      (left.kind == KLVAL_STRING && right.kind == KLVAL_STRING)) {
    kl_assert(oristktop == klgen_stacktop(gen), "");
    /* try to compute at compile time */
    if (klcodeval_isnumber(left) && bincst->op != KLTK_CONCAT) {
      return klgen_tryarithcomptime(gen, bincst, left, right, target);
    }
    if (left.kind == KLVAL_STRING && bincst->op == KLTK_CONCAT) {
      char* res = klstrtab_concat(gen->strtab, left.string, right.string);
      klgen_oomifnull(gen, res);
      KlStrDesc str = { .id = klstrtab_stringid(gen->strtab, res),
        .length = left.string.length + right.string.length };
      return klcodeval_string(str);
    }
    /* can not apply compile time computation, fall through */
  }
  klgen_putonstack(gen, &left, klgen_cstposition(bincst->loperand));
  if (right.kind == KLVAL_STACK) {
    KlInstruction inst = klgen_bininst(bincst, target, left.index, right.index);
    klgen_emit(gen, inst, klgen_cstposition(bincst));
    klgen_stackfree(gen, oristktop == target ? target + 1 : oristktop);
    return klcodeval_stack(target);
  } else {
    return klgen_exprbinrightnonstk(gen, bincst, left, right, target, oristktop);
  }
}

static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target, size_t oristktop) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  size_t finstktop = oristktop == target ? target + 1 : oristktop;
  switch (right.kind) {
    case KLVAL_INTEGER: {
      if (bincst->op == KLTK_CONCAT) {
        klgen_putonstack(gen, &right, klgen_cstposition(bincst->roperand));
        klgen_emit(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
      } else if (klinst_inrange(right.intval, 8) && bincst->op != KLTK_DIV) {
        klgen_emit(gen, klgen_bininsti(bincst, target, left.index, right.intval), klgen_cstposition(bincst));
      } else {
        KlConstant con = { .type = KL_INT, .intval = right.intval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(gen, conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_emit(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t stktop = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
          klgen_emit(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
        }
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_FLOAT: {
      if (bincst->op == KLTK_CONCAT) {
        klgen_putonstack(gen, &right, klgen_cstposition(bincst->roperand));
        klgen_emit(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
      } else {
        KlConstant con = { .type = KL_FLOAT, .intval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(gen, conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_emit(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t stktop = klgen_stackalloc1(gen);
          klgen_emit(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
          klgen_emit(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
        }
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_STRING: {
      KlConstant con = { .type = KL_STRING, .string = right.string };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(gen, conent);
      if (klinst_inurange(conent->index, 8) && bincst->op != KLTK_CONCAT) {
        klgen_emit(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
      } else {
        size_t stktop = klgen_stackalloc1(gen);
        klgen_emit(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
        klgen_emit(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_REF:
    case KLVAL_BOOL:
    case KLVAL_NIL: {
      klgen_putonstack(gen, &right, klgen_cstposition(bincst->roperand));
      klgen_emit(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

KlCodeVal klgen_exprbin(KlGenUnit* gen, KlCstBin* bincst, size_t target) {
  if (kltoken_isarith(bincst->op) || bincst->op == KLTK_CONCAT) {
    size_t leftid = klgen_stacktop(gen);
    KlCodeVal left = klgen_expr(gen, bincst->loperand);
    if (klcodeval_isnumber(left) || left.kind == KLVAL_STRING) {
      return klgen_exprbinleftliteral(gen, bincst, left, target);
    } else {
      klgen_putonstack(gen, &left, klgen_cstposition(bincst->loperand));
    }
    /* now left is on stack */
    KlCodeVal right = klgen_expr(gen, bincst->roperand);
    if (right.kind != KLVAL_STACK)
      return klgen_exprbinrightnonstk(gen, bincst, left, right, target, leftid);
    /* now both are on stack */
    KlInstruction inst = klgen_bininst(bincst, target, left.index, right.index);
    klgen_emit(gen, inst, klgen_cstposition(bincst));
    klgen_stackfree(gen, leftid == target ? target + 1 : leftid);
    return klcodeval_stack(leftid);
  } else {  /* else is boolean expression */
    KlCodeVal res = klgen_exprboolval(gen, klcst(bincst), target);
    if (klcodeval_isconstant(res)) return res;
    return klcodeval_stack(target);
  }
}

KlCodeVal klgen_exprpost(KlGenUnit* gen, KlCstPost* postcst, size_t target, bool append_target) {
  switch (postcst->op) {
    case KLTK_CALL: {
      klgen_call(gen, postcst, 1, target);
      return klcodeval_stack(target);
    }
    case KLTK_INDEX: {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal indexable = klgen_expr(gen, postcst->operand);
      klgen_putonstack(gen, &indexable, klgen_cstposition(postcst->operand));
      KlCodeVal index = klgen_expr(gen, postcst->post);
      if (index.kind == KLVAL_INTEGER && klinst_inrange(index.intval, 8)) {
        klgen_emit(gen, klinst_indexi(target, indexable.index, index.intval), klgen_cstposition(postcst));
      } else {
        klgen_putonstack(gen, &index, klgen_cstposition(postcst->post));
        klgen_emit(gen, klinst_index(target, indexable.index, index.index), klgen_cstposition(postcst));
      }
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLTK_APPEND: {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal appendable = klgen_expr(gen, postcst->operand);
      klgen_putonstack(gen, &appendable, klgen_cstposition(postcst->operand));
      size_t base = klgen_stacktop(gen);
      size_t narg = klgen_passargs(gen, postcst->post);
      klgen_emit(gen, klinst_append(appendable.index, base, narg), klgen_cstposition(postcst));
      if (append_target && target != appendable.index)
        klgen_emitmove(gen, target, appendable.index, 1, klgen_cstposition(postcst));

      klgen_stackfree(gen, target == stktop ? target + 1 : stktop);
      return klcodeval_stack(append_target ? target : appendable.index);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static void klgen_exprnew(KlGenUnit* gen, KlCstNew* newcst, size_t target) {
  size_t oristktop = klgen_stacktop(gen);
  KlCodeVal klclass = klgen_expr(gen, newcst->klclass);
  klgen_putonstack(gen, &klclass, klgen_cstposition(newcst->klclass));
  klgen_emit(gen, klinst_newobj(target, klclass.index), klgen_cstposition(newcst));
  if (target == klgen_stacktop(gen))
    klgen_stackalloc1(gen);
  size_t stktop = klgen_stackalloc1(gen);
  klgen_emitmove(gen, stktop, target, 1, klgen_cstposition(newcst));
  size_t narg = klgen_passargs(gen, newcst->args);
  KlConstant con = { .type = KL_STRING, .string = gen->string.constructor };
  KlConEntry* conent = klcontbl_get(gen->contbl, &con);
  klgen_oomifnull(gen, conent);
  klgen_emitmethod(gen, stktop, conent->index, narg, 0, stktop, klgen_cstposition(newcst));
  klgen_stackfree(gen, oristktop == target ? target + 1 : oristktop);
}

static void klgen_exprdot(KlGenUnit* gen, KlCstDot* dotcst, size_t target) {
  size_t stktop = klgen_stacktop(gen);
  KlCodeVal obj = klgen_expr(gen, dotcst->operand);
  KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  if (obj.kind == KLVAL_REF) {
    if (klinst_inurange(conent->index, 8)) {
      klgen_emit(gen, klinst_refgetfieldc(target, obj.index, conent->index), klgen_cstposition(dotcst));
    } else {
      size_t stkid = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(stkid, conent->index), klgen_cstposition(dotcst));
      klgen_emit(gen, klinst_refgetfieldr(target, obj.index, stkid), klgen_cstposition(dotcst));
    }
  } else {
    klgen_putonstack(gen, &obj, klgen_cstposition(dotcst->operand));
    if (klinst_inurange(conent->index, 8)) {
      klgen_emit(gen, klinst_getfieldc(target, obj.index, conent->index), klgen_cstposition(dotcst));
    } else {
      size_t stkid = klgen_stackalloc1(gen);
      klgen_emit(gen, klinst_loadc(stkid, conent->index), klgen_cstposition(dotcst));
      klgen_emit(gen, klinst_getfieldr(target, obj.index, stkid), klgen_cstposition(dotcst));
    }
  }
  klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
}


KlCodeVal klgen_expr(KlGenUnit* gen, KlCst* cst) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_ARR: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprarr(gen, klcast(KlCstArray*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_ARRGEN: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprarrgen(gen, klcast(KlCstArrayGenerator*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_WHERE: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprwhere(gen, klcast(KlCstWhere*, cst), 1, target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_MAP: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprmap(gen, klcast(KlCstMap*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_CLASS: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprclass(gen, klcast(KlCstClass*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_CONSTANT: {
      return klgen_constant(gen, klcast(KlCstConstant*, cst));
    }
    case KLCST_EXPR_ID: {
      return klgen_identifier(gen, klcast(KlCstIdentifier*, cst));
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    case KLCST_EXPR_TUPLE: {
      return klgen_tuple(gen, klcast(KlCstTuple*, cst));
    }
    case KLCST_EXPR_PRE: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprpre(gen, klcast(KlCstPre*, cst), target);
    }
    case KLCST_EXPR_NEW: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprnew(gen, klcast(KlCstNew*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_YIELD: {
      size_t stkid = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), 1);
      return klcodeval_stack(stkid);
    }
    case KLCST_EXPR_POST: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprpost(gen, klcast(KlCstPost*, cst), target, false);
    }
    case KLCST_EXPR_DOT: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprdot(gen, klcast(KlCstDot*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_FUNC: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprfunc(gen, klcast(KlCstFunc*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_BIN: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      return klgen_exprbin(gen, klcast(KlCstBin*, cst), target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlCst* cst, size_t target) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_ARR: {
      klgen_exprarr(gen, klcast(KlCstArray*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_ARRGEN: {
      klgen_exprarrgen(gen, klcast(KlCstArrayGenerator*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_WHERE: {
      klgen_exprwhere(gen, klcast(KlCstWhere*, cst), 1, target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_MAP: {
      klgen_exprmap(gen, klcast(KlCstMap*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_CLASS: {
      klgen_exprclass(gen, klcast(KlCstClass*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_CONSTANT: {
      return klgen_constant(gen, klcast(KlCstConstant*, cst));
    }
    case KLCST_EXPR_ID: {
      size_t stktop = klgen_stacktop(gen);
      KlCstIdentifier* idcst = klcast(KlCstIdentifier*, cst);
      KlSymbol* symbol = klgen_getsymbol(gen, idcst->id);
      if (symbol) {
        KlCodeVal idval = klcodeval_index(symbol->attr.kind, symbol->attr.idx);
        klgen_loadval(gen, target, idval, klgen_cstposition(cst));
      } else {  /* else is global variable */
        KlConstant con = { .type = KL_STRING, .string = idcst->id };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(gen, conent);
        klgen_emit(gen, klinst_loadglobal(target, conent->index), klgen_cstposition(idcst));
      }
      klgen_stackfree(gen, stktop == target ? target + 1 : stktop);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    case KLCST_EXPR_TUPLE: {
      return klgen_tuple_target(gen, klcast(KlCstTuple*, cst), target);
    }
    case KLCST_EXPR_PRE: {
      return klgen_exprpre(gen, klcast(KlCstPre*, cst), target);
    }
    case KLCST_EXPR_NEW: {
      klgen_exprnew(gen, klcast(KlCstNew*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_YIELD: {
      size_t stktop = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), 1);
      if (target != stktop)
        klgen_emitmove(gen, target, stktop, 1, klgen_cstposition(cst));
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_POST: {
      return klgen_exprpost(gen, klcast(KlCstPost*, cst), target, true);
    }
    case KLCST_EXPR_DOT: {
      klgen_exprdot(gen, klcast(KlCstDot*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_FUNC: {
      klgen_exprfunc(gen, klcast(KlCstFunc*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_BIN: {
      return klgen_exprbin(gen, klcast(KlCstBin*, cst), target);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}
