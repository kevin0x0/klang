#include "klang/include/code/klcodeval.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klgen_expr.h"
#include "klang/include/code/klgen_exprbool.h"
#include "klang/include/code/klgen_stmt.h"
#include "klang/include/vm/klinst.h"

static KlCodeVal klgen_exprbinleftliteral(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, size_t target);
static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target);

void klgen_exprarr(KlGenUnit* gen, KlCstArray* arrcst, size_t target) {
  size_t argbase = klgen_stacktop(gen);
  size_t nval = klgen_passargs(gen, arrcst->vals);
  klgen_pushinst(gen, klinst_mkarray(target, argbase, nval), klgen_cstposition(arrcst));
}

void klgen_exprarrgen(KlGenUnit* gen, KlCstArrayGenerator* arrgencst, size_t target) {
  kltodo("implement arrgen");
  if (target >= klgen_stacktop(gen))
    klgen_stackfree(gen, target + 1);
  klgen_newsymbol(gen, arrgencst->arrid, target, klcst_begin(arrgencst));
  KlCodeVal* prevcjmp = gen->info.continuejmp;
  KlCodeVal* prevbjmp = gen->info.breakjmp;
  gen->info.continuejmp = NULL;
  gen->info.breakjmp = NULL;
  klgen_stmtblock(gen, arrgencst->block);
  gen->info.continuejmp = prevcjmp;
  gen->info.breakjmp = prevbjmp;
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
  size_t oristktop = klgen_stacktop(gen);
  klgen_pushinst(gen, klinst_mkmap(target, oristktop, sizefield), klgen_cstposition(mapcst));
  size_t stktop;
  if (oristktop <= target) {
    klgen_stackfree(gen, target + 1);
    stktop = klgen_stacktop(gen);
  } else {
    stktop = oristktop;
  }
  size_t npair = mapcst->npair;
  for (size_t i = 0; i < npair; ++i) {
    KlCst* key = mapcst->keys[i];
    KlCst* val = mapcst->vals[i];
    KlCodeVal keypos = klgen_expr(gen, key);
    if (keypos.kind == KLVAL_INTEGER && klinst_inrange(keypos.intval, 8)) {
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putinstack(gen, &valpos, klgen_cstposition(val));
      klgen_pushinst(gen, klinst_indexasi(valpos.index, target, keypos.intval), klgen_cstposition(mapcst));
    } else {
      klgen_putinstack(gen, &keypos, klgen_cstposition(key));
      KlCodeVal valpos = klgen_expr(gen, val);
      klgen_putinstack(gen, &valpos, klgen_cstposition(val));
      klgen_pushinst(gen, klinst_indexas(valpos.index, target, keypos.index), klgen_cstposition(mapcst));
    }
    klgen_stackfree(gen, stktop);
  }
  klgen_stackfree(gen, stktop);
}

static void klgen_exprclasspost(KlGenUnit* gen, KlCstClass* classcst, size_t target) {
  size_t nfield = classcst->nfield;
  size_t stktop = klgen_stacktop(gen);
  for (size_t i = 0; i < nfield; ++i) {
    KlCstClassFieldDesc field = classcst->fields[i];
    KlConstant constant = { .type = KL_STRING, .string = field.name };
    KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
    klgen_oomifnull(conent);
    if (field.shared) {
      KlCodeVal val = klgen_expr(gen, classcst->vals[i]);
      klgen_putinstack(gen, &val, klgen_cstposition(classcst->vals[i]));
      if (klinst_inurange(conent->index, 8)) {
        klgen_pushinst(gen, klinst_setfieldc(val.index, target, conent->index), klgen_cstposition(classcst));
      } else {
        size_t stkid = klgen_stackalloc1(gen);
        klgen_pushinst(gen, klinst_loadc(stkid, conent->index), klgen_cstposition(classcst));
        klgen_pushinst(gen, klinst_setfieldr(val.index, target, stkid), klgen_cstposition(classcst));
      }
    } else {
      klgen_pushinst(gen, klinst_newlocal(target, conent->index), klgen_cstposition(classcst));
    }
    klgen_stackfree(gen, stktop);
  }
}

void klgen_exprclass(KlGenUnit* gen, KlCstClass* classcst, size_t target) {
  size_t stktop = klgen_stacktop(gen);
  size_t class_size = abovelog2(classcst->nfield);
  if (classcst->baseclass) {
    /* base is specified */
    KlCodeVal base = klgen_exprtarget(gen, classcst->baseclass, stktop);
    if (klcodeval_isconstant(base))
      klgen_putinstktop(gen, &base, klgen_cstposition(classcst->baseclass));
    kl_assert(base.index == stktop, "");
    klgen_pushinst(gen, klinst_mkclass(target, stktop, true, class_size), klgen_cstposition(classcst));
  } else {
    klgen_pushinst(gen, klinst_mkclass(target, stktop, false, class_size), klgen_cstposition(classcst));
  }
  klgen_stackfree(gen, stktop > target ? stktop : target + 1);
  klgen_exprclasspost(gen, classcst, target);
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
    klgen_oomifnull(conent);
    klgen_pushinst(gen, klinst_loadglobal(stkid, conent->index), klgen_cstposition(idcst));
    return klcodeval_stack(stkid);
  }
}

void klgen_method(KlGenUnit* gen, KlCst* objcst, KlStrDesc method, KlCst* args, KlFilePosition position, size_t nret) {
  size_t base = klgen_stacktop(gen);
  KlCodeVal obj = klgen_exprtarget(gen, objcst, base);
  if (klcodeval_isconstant(obj))
    klgen_putinstktop(gen, &obj, klgen_cstposition(objcst));
  size_t narg = klgen_passargs(gen, args);
  KlConstant con = { .type = KL_STRING, .string = method };
  KlConEntry* conent = klcontbl_get(gen->contbl, &con);
  klgen_oomifnull(conent);
  klgen_pushinstmethod(gen, base, conent->index, narg, nret, position);
  klgen_stackfree(gen, base + nret);
}

void klgen_call(KlGenUnit* gen, KlCstPost* callcst, size_t nret) {
  if (klcst_kind(callcst->operand) == KLCST_EXPR_DOT) {
    KlCstDot* dotcst = klcast(KlCstDot*, callcst->operand);
    klgen_method(gen, dotcst->operand, dotcst->field, callcst->post, klgen_cstposition(callcst), nret);
  }
  size_t base = klgen_stacktop(gen);
  KlCodeVal callable = klgen_exprtarget(gen, callcst->operand, base);
  if (klcodeval_isconstant(callable))
    klgen_putinstktop(gen, &callable, klgen_cstposition(callcst->operand));
  size_t narg = klgen_passargs(gen, callcst->post);
  klgen_pushinst(gen, klinst_call(callable.index, narg, nret), klgen_cstposition(callcst));
  klgen_stackfree(gen, base + nret);
}

void klgen_multival(KlGenUnit* gen, KlCst* cst, size_t nval) {
  kl_assert(nval > 0, "");
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_YIELD: {
      klgen_expryield(gen, klcast(KlCstYield*, cst), nval);
      break;
    }
    case KLCST_EXPR_CALL: {
      klgen_call(gen, klcast(KlCstPost*, cst), nval);
      break;
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    default: {
      size_t base = klgen_stacktop(gen);
      KlCodeVal res = klgen_exprtarget(gen, cst, base);
      if (klcodeval_isconstant(res))
        klgen_putinstktop(gen, &res, klgen_cstposition(cst));
      if (nval != 1) {
        klgen_pushinst(gen, klinst_loadnil(res.index + 1, nval - 1), klgen_cstposition(cst));
        klgen_stackalloc(gen, nval - 1);
      }
      break;
    }
  }
}

void klgen_tuple(KlGenUnit* gen, KlCstTuple* tuplecst, size_t nwanted) {
  size_t nvalid = nwanted < tuplecst->nelem ? nwanted : tuplecst->nelem;
  if (nvalid == 0) {
    if (nwanted == 0) return;
    size_t stktop = klgen_stacktop(gen);
    klgen_pushinst(gen, klinst_loadnil(stktop, nwanted), klgen_cstposition(tuplecst));
    klgen_stackalloc(gen, nwanted);
    return;
  }
  KlCst** exprs = tuplecst->elems;
  size_t count = nvalid - 1;
  for (size_t i = 0; i < count; ++i) {
    KlCodeVal res = klgen_exprtarget(gen, exprs[i], klgen_stacktop(gen));
    if (klcodeval_isconstant(res))
      klgen_putinstktop(gen, &res, klgen_cstposition(exprs[i]));
  }
  klgen_multival(gen, exprs[count], nwanted - count);
}

size_t klgen_passargs(KlGenUnit* gen, KlCst* args) {
  if (klcst_kind(args) == KLCST_EXPR_TUPLE) {
    size_t narg = klcast(KlCstTuple*, args)->nelem;
    klgen_tuple(gen, klcast(KlCstTuple*, args), narg);
    return narg;
  } else {  /* else is a normal expression */
    KlCodeVal res = klgen_exprtarget(gen, args, klgen_stacktop(gen));
    if (klcodeval_isconstant(res))
      klgen_putinstktop(gen, &res, klgen_cstposition(args));
    return 1;
  }
}

KlCodeVal klgen_exprpre(KlGenUnit* gen, KlCstPre* precst) {
  switch (precst->op) {
    case KLTK_MINUS: {
      size_t stkid = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, precst->operand);
      if (val.kind == KLVAL_INTEGER) {
        return klcodeval_integer(-val.intval);
      } else if (val.kind == KLVAL_FLOAT) {
        return klcodeval_float(-val.floatval);
      }
      KlFilePosition pos = klgen_cstposition(precst);
      klgen_putinstack(gen, &val, pos);
      klgen_pushinst(gen, klinst_neg(stkid, val.index), pos);
      klgen_stackfree(gen, stkid + 1);
      return klcodeval_stack(stkid);
    }
    case KLTK_ASYNC: {
      size_t stkid = klgen_stacktop(gen);
      KlCodeVal val = klgen_expr(gen, precst->operand);
      KlFilePosition pos = klgen_cstposition(precst);
      klgen_putinstack(gen, &val, pos);
      klgen_pushinst(gen, klinst_async(stkid, val.index), pos);
      klgen_stackfree(gen, stkid + 1);
      return klcodeval_stack(stkid);
    }
    case KLTK_NOT: {
      size_t stkid = klgen_stacktop(gen);
      KlCodeVal res = klgen_exprboolval(gen, klcst(precst), stkid);
      if (klcodeval_isconstant(res)) return res;
      kl_assert(stkid + 1 == klgen_stacktop(gen), "");
      return klcodeval_stack(stkid);
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
    case KLTK_DIV:
      return klinst_divi(stkid, leftid, imm);
    case KLTK_MOD:
      return klinst_modi(stkid, leftid, imm);
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
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlCodeVal klgen_arithcomptime(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target) {
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
        if (right.intval == 0) {
          klgen_error(gen, klcst_begin(bincst), klcst_end(bincst), "divided by zero");
          return klcodeval_float(1.0);
        }
        return klcodeval_integer(left.intval / right.intval);
      }
      case KLTK_MOD: {
        if (right.intval == 0) {
          klgen_error(gen, klcst_begin(bincst), klcst_end(bincst), "divided by zero");
          return klcodeval_float(1.0);
        }
        return klcodeval_integer(left.intval % right.intval);
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
        /* why not compute this in compile time ? */
        size_t finstktop = klgen_stacktop(gen) <= target ? target + 1 : klgen_stacktop(gen);
        klgen_putinstack(gen, &left, klgen_cstposition(bincst->loperand));
        KlConstant constant = { .type = KL_FLOAT, .floatval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
        klgen_oomifnull(conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_pushinst(gen, klinst_divc(target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t tmp = klgen_stackalloc1(gen);
          klgen_pushinst(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(bincst->roperand));
          klgen_pushinst(gen, klinst_div(target, left.index, tmp), klgen_cstposition(bincst));
        }
        klgen_stackfree(gen, finstktop);
        return klcodeval_stack(target);
      }
      case KLTK_MOD: {
        size_t finstktop = klgen_stacktop(gen) <= target ? target + 1 : klgen_stacktop(gen);
        klgen_putinstack(gen, &left, klgen_cstposition(bincst->loperand));
        KlConstant constant = { .type = KL_FLOAT, .floatval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
        klgen_oomifnull(conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_pushinst(gen, klinst_modc(target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t tmp = klgen_stackalloc1(gen);
          klgen_pushinst(gen, klinst_loadc(tmp, conent->index), klgen_cstposition(bincst->roperand));
          klgen_pushinst(gen, klinst_mod(target, left.index, tmp), klgen_cstposition(bincst));
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
  size_t currcodesize = klgen_currcodesize(gen);
  /* we put left on the stack first */
  KlCodeVal leftonstack = left;
  klgen_putinstack(gen, &leftonstack, klgen_cstposition(bincst->loperand));
  size_t stksize_before_evaluete_right = klgen_currcodesize(gen);
  KlCodeVal right = klgen_expr(gen, bincst->roperand);
  if ((klcodeval_isnumber(right) && klcodeval_isnumber(left)) ||
      (left.kind == KLVAL_STRING && right.kind == KLVAL_STRING)) {
    /* try to compute at compile time */
    if (klcodeval_isnumber(left) && bincst->op != KLTK_CONCAT) {
      if (stksize_before_evaluete_right == klgen_currcodesize(gen))
        klgen_popinstto(gen, currcodesize);   /* pop the instruction that put left on stack */
      klgen_stackfree(gen, oristktop);
      return klgen_arithcomptime(gen, bincst, left, right, target);
    }
    if (left.kind == KLVAL_STRING && bincst->op == KLTK_CONCAT) {
      if (stksize_before_evaluete_right == klgen_currcodesize(gen))
        klgen_popinstto(gen, currcodesize);   /* pop the instruction that put left on stack */
      klgen_stackfree(gen, oristktop);
      char* res = klstrtab_concat(gen->strtab, left.string, right.string);
      klgen_oomifnull(res);
      KlStrDesc str = { .id = klstrtab_stringid(gen->strtab, res),
        .length = left.string.length + right.string.length };
      return klcodeval_string(str);
    }
    /* can not apply compile time computation, fall through */
  }
  if (right.kind == KLVAL_STACK) {
    /* now we are sure that left should indeed be put on the stack */
    KlInstruction inst = klgen_bininst(bincst, target, leftonstack.index, right.index);
    klgen_pushinst(gen, inst, klgen_cstposition(bincst));
    klgen_stackfree(gen, oristktop <= target ? target + 1 : oristktop);
    return klcodeval_stack(target);
  } else {
    return klgen_exprbinrightnonstk(gen, bincst, leftonstack, right, target);
  }
}

static KlCodeVal klgen_exprbinrightnonstk(KlGenUnit* gen, KlCstBin* bincst, KlCodeVal left, KlCodeVal right, size_t target) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  size_t finstktop = target == klgen_stacktop(gen) <= target ? target + 1 : klgen_stacktop(gen);
  switch (right.kind) {
    case KLVAL_INTEGER: {
      if (bincst->op == KLTK_CONCAT) {
        klgen_putinstack(gen, &right, klgen_cstposition(bincst->roperand));
        klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
      } else if (klinst_inrange(right.intval, 8)) {
        klgen_pushinst(gen, klgen_bininsti(bincst, target, left.index, right.intval), klgen_cstposition(bincst));
      } else {
        KlConstant con = { .type = KL_INT, .intval = right.intval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_pushinst(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t stktop = klgen_stackalloc1(gen);
          klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
          klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
        }
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_FLOAT: {
      if (bincst->op == KLTK_CONCAT) {
        klgen_putinstack(gen, &right, klgen_cstposition(bincst->roperand));
        klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
      } else {
        KlConstant con = { .type = KL_FLOAT, .intval = right.floatval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(conent);
        if (klinst_inurange(conent->index, 8)) {
          klgen_pushinst(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
        } else {
          size_t stktop = klgen_stackalloc1(gen);
          klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
          klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
        }
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_STRING: {
      KlConstant con = { .type = KL_STRING, .string = right.string };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(conent);
      if (klinst_inurange(conent->index, 8) && bincst->op != KLTK_CONCAT) {
        klgen_pushinst(gen, klgen_bininstc(bincst, target, left.index, conent->index), klgen_cstposition(bincst));
      } else {
        size_t stktop = klgen_stackalloc1(gen);
        klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(bincst->roperand));
        klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, stktop), klgen_cstposition(bincst));
      }
      klgen_stackfree(gen, finstktop);
      return klcodeval_stack(target);
    }
    case KLVAL_REF:
    case KLVAL_BOOL:
    case KLVAL_NIL: {
      klgen_putinstack(gen, &right, klgen_cstposition(bincst->roperand));
      klgen_pushinst(gen, klgen_bininst(bincst, target, left.index, right.index), klgen_cstposition(bincst));
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
      klgen_putinstack(gen, &left, klgen_cstposition(bincst->loperand));
    }
    /* now left is on stack */
    KlCodeVal right = klgen_expr(gen, bincst->roperand);
    if (right.kind != KLVAL_STACK)
      return klgen_exprbinrightnonstk(gen, bincst, left, right, target);
    /* now both are on stack */
    KlInstruction inst = klgen_bininst(bincst, leftid, left.index, right.index);
    klgen_pushinst(gen, inst, klgen_cstposition(bincst));
    klgen_stackfree(gen, leftid + 1);
    return klcodeval_stack(leftid);
  } else {  /* else is boolean expression */
    kltodo("implement boolean expression");
    size_t stkid = klgen_stacktop(gen);
    KlCodeVal res = klgen_exprboolval(gen, klcst(bincst), stkid);
    if (klcodeval_isconstant(res)) return res;
    kl_assert(stkid + 1 == klgen_stacktop(gen), "");
    return klcodeval_stack(stkid);
  }
}

KlCodeVal klgen_exprpost(KlGenUnit* gen, KlCstPost* postcst) {
  switch (postcst->op) {
    case KLTK_CALL: {
      size_t stkid = klgen_stacktop(gen);
      klgen_call(gen, postcst, 1);
      return klcodeval_stack(stkid);
    }
    case KLTK_INDEX: {
      size_t stkid = klgen_stacktop(gen);
      KlCodeVal indexable = klgen_expr(gen, postcst->operand);
      klgen_putinstack(gen, &indexable, klgen_cstposition(postcst->operand));
      KlCodeVal index = klgen_expr(gen, postcst->post);
      if (index.kind == KLVAL_INTEGER && klinst_inrange(index.intval, 8)) {
        klgen_pushinst(gen, klinst_indexi(stkid, indexable.index, index.intval), klgen_cstposition(postcst));
      } else {
        klgen_putinstack(gen, &index, klgen_cstposition(postcst->post));
        klgen_pushinst(gen, klinst_index(stkid, indexable.index, index.index), klgen_cstposition(postcst));
      }
      klgen_stackfree(gen, stkid + 1);
      return klcodeval_stack(stkid);
    }
    case KLTK_APPEND: {
      size_t stktop = klgen_stacktop(gen);
      KlCodeVal appendable = klgen_expr(gen, postcst->operand);
      klgen_putinstack(gen, &appendable, klgen_cstposition(postcst->operand));
      size_t base = klgen_stacktop(gen);
      size_t narg = klgen_passargs(gen, postcst->post);
      klgen_pushinst(gen, klinst_append(appendable.index, base, narg), klgen_cstposition(postcst));
      klgen_stackfree(gen, appendable.index == stktop ? stktop + 1 : stktop);
      return klcodeval_stack(appendable.index);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static void klgen_exprnew(KlGenUnit* gen, KlCstNew* newcst, size_t target) {
  KlCodeVal klclass = klgen_expr(gen, newcst->klclass);
  klgen_putinstack(gen, &klclass, klgen_cstposition(newcst->klclass));
  klgen_pushinst(gen, klinst_newobj(target, klclass.index), klgen_cstposition(newcst));
  size_t oristktop = klgen_stacktop(gen);
  size_t stktop;
  if (oristktop == target) {
    klgen_stackalloc1(gen);
    stktop = klgen_stacktop(gen);
  } else {
    stktop = oristktop;
  }
  klgen_pushinst(gen, klinst_move(stktop, target), klgen_cstposition(newcst));
  klgen_stackfree(gen, stktop + 1);
  size_t narg = klgen_passargs(gen, newcst->args);
  KlConstant con = { .type = KL_STRING, .string = gen->string.constructor };
  KlConEntry* conent = klcontbl_get(gen->contbl, &con);
  klgen_oomifnull(conent);
  klgen_pushinstmethod(gen, stktop, conent->index, narg, 0, klgen_cstposition(newcst));
  klgen_stackfree(gen, oristktop);
}

static void klgen_exprdot(KlGenUnit* gen, KlCstDot* dotcst, size_t target) {
  size_t stktop = klgen_stacktop(gen);
  KlCodeVal obj = klgen_expr(gen, dotcst->operand);
  KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(conent);
  if (obj.kind == KLVAL_REF) {
    if (klinst_inurange(conent->index, 8)) {
      klgen_pushinst(gen, klinst_refgetfieldc(target, obj.index, conent->index), klgen_cstposition(dotcst));
    } else {
      klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(dotcst));
      klgen_pushinst(gen, klinst_refgetfieldr(target, obj.index, stktop), klgen_cstposition(dotcst));
    }
    klgen_stackfree(gen, stktop);
  } else {
    klgen_putinstack(gen, &obj, klgen_cstposition(dotcst->operand));
    if (klinst_inurange(conent->index, 8)) {
      klgen_pushinst(gen, klinst_getfieldc(target, obj.index, conent->index), klgen_cstposition(dotcst));
    } else {
      size_t stktop = klgen_stacktop(gen);
      klgen_pushinst(gen, klinst_loadc(stktop, conent->index), klgen_cstposition(dotcst));
      klgen_pushinst(gen, klinst_getfieldr(target, obj.index, stktop), klgen_cstposition(dotcst));
    }
    klgen_stackfree(gen, stktop);
  }
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
      return klgen_tuple_as_singleval(gen, klcast(KlCstTuple*, cst));
    }
    case KLCST_EXPR_PRE: {
      return klgen_exprpre(gen, klcast(KlCstPre*, cst));
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
      return klgen_exprpost(gen, klcast(KlCstPost*, cst));
    }
    case KLCST_EXPR_DOT: {
      size_t target = klgen_stacktop(gen);   /* here we generate the value on top of the stack */
      klgen_exprdot(gen, klcast(KlCstDot*, cst), target);
      return klcodeval_stack(target);
    }
    case KLCST_EXPR_FUNC: {
      kltodo("implement func");
    }
    case KLCST_EXPR_BIN: {
      return klgen_exprbin(gen, klcast(KlCstBin*, cst));
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static inline void klgen_retarget(KlGenUnit* gen, size_t target) {
  KlInstruction* pc = klinstarr_access(&gen->code, klgen_currcodesize(gen) - 1);
  uint8_t opcode = KLINST_GET_OPCODE(*pc);
  uint32_t rest = KLINST_AX_GETX(*pc);
  *pc = klinst_AX(opcode, target, rest);
}

KlCodeVal klgen_exprtarget(KlGenUnit* gen, KlCst* cst, size_t target) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_ARR: {
      klgen_exprarr(gen, klcast(KlCstArray*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_ARRGEN: {
      klgen_exprarrgen(gen, klcast(KlCstArrayGenerator*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_MAP: {
      klgen_exprmap(gen, klcast(KlCstMap*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_CLASS: {
      klgen_exprclass(gen, klcast(KlCstClass*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_CONSTANT: {
      return klgen_constant(gen, klcast(KlCstConstant*, cst));
    }
    case KLCST_EXPR_ID: {
      KlCodeVal id = klgen_identifier(gen, klcast(KlCstIdentifier*, cst));
      if (id.kind == KLVAL_STACK && id.index == klgen_stacktop(gen)) {
        /* global variable */
        klgen_retarget(gen, target);
      } else {
        klgen_loadval(gen, target, id, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    case KLCST_EXPR_TUPLE: {
      KlCodeVal val = klgen_tuple_as_singleval_target(gen, klcast(KlCstTuple*, cst), target);
      if (klcodeval_isconstant(val))
        return val;
      if (val.kind == KLVAL_STACK && val.index == klgen_stacktop(gen)) {
        klgen_retarget(gen, target);
      } else {
        klgen_loadval(gen, target, val, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    case KLCST_EXPR_PRE: {
      KlCstPre* pre = klcast(KlCstPre*, cst);
      if (pre->op == KLTK_NOT)
        return klgen_exprboolval(gen, cst, target);
      KlCodeVal val = klgen_exprpre(gen, pre);
      if (klcodeval_isconstant(val))
        return val;
      if (val.kind == KLVAL_STACK && val.index == klgen_stacktop(gen)) {
        klgen_retarget(gen, target);
      } else {
        klgen_loadval(gen, target, val, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    case KLCST_EXPR_NEW: {
      klgen_exprnew(gen, klcast(KlCstNew*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_YIELD: {
      size_t stkid = klgen_stacktop(gen);
      klgen_expryield(gen, klcast(KlCstYield*, cst), 1);
      klgen_loadval(gen, target, klcodeval_stack(stkid), klgen_cstposition(cst));
      return klcodeval_none();
    }
    case KLCST_EXPR_POST: {
      KlCodeVal val = klgen_exprpost(gen, klcast(KlCstPost*, cst));
      if (klcodeval_isconstant(val))
        return val;
      if (val.kind == KLVAL_STACK && val.index == klgen_stacktop(gen)) {
        klgen_retarget(gen, target);
      } else {
        klgen_loadval(gen, target, val, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    case KLCST_EXPR_DOT: {
      klgen_exprdot(gen, klcast(KlCstDot*, cst), target);
      return klcodeval_none();
    }
    case KLCST_EXPR_FUNC: {
      kltodo("implement func");
    }
    case KLCST_EXPR_BIN: {
      KlCstBin* bincst = klcast(KlCstBin*, cst);
      if (bincst->op == KLTK_AND || bincst->op == KLTK_OR || kltoken_isrelation(bincst->op))
        return klgen_exprboolval(gen, cst, target);
      KlCodeVal val = klgen_exprbin(gen, bincst);
      if (klcodeval_isconstant(val))
        return val;
      if (val.kind == KLVAL_STACK && val.index == klgen_stacktop(gen)) {
        klgen_retarget(gen, target);
      } else {
        klgen_loadval(gen, target, val, klgen_cstposition(cst));
      }
      return klcodeval_none();
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}
