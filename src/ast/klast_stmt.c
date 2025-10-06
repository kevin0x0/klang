#include "include/ast/klast.h"
#include "include/misc/klutils.h"
#include <stdlib.h>


static void stmtlet_destroy(KlAstStmtLet* stmtlet);
static void stmtmethod_destroy(KlAstStmtMethod* stmtmethod);
static void stmtmatch_destroy(KlAstStmtMatch* stmtmatch);
static void stmtlocalfunc_destroy(KlAstStmtLocalDefinition* stmtlocalfunc);
static void stmtassign_destroy(KlAstStmtAssign* stmtassign);
static void stmtexpr_destroy(KlAstStmtExpr* stmtexpr);
static void stmtif_destroy(KlAstStmtIf* stmtif);
static void stmtvfor_destroy(KlAstStmtVFor* stmtvfor);
static void stmtifor_destroy(KlAstStmtIFor* stmtifor);
static void stmtgfor_destroy(KlAstStmtGFor* stmtgfor);
static void stmtwhile_destroy(KlAstStmtWhile* stmtwhile);
static void stmtlist_destroy(KlAstStmtList* stmtblock);
static void stmtrepeat_destroy(KlAstStmtRepeat* stmtrepeat);
static void stmtreturn_destroy(KlAstStmtReturn* stmtreturn);
static void stmtbreak_destroy(KlAstStmtBreak* stmtbreak);
static void stmtcontinue_destroy(KlAstStmtContinue* stmtcontinue);

static const KlAstInfo stmtlet_vfunc = { .destructor = (KlAstDelete)stmtlet_destroy, .kind = KLAST_STMT_LET };
static const KlAstInfo stmtmethod_vfunc = { .destructor = (KlAstDelete)stmtmethod_destroy, .kind = KLAST_STMT_METHOD };
static const KlAstInfo stmtmatch_vfunc = { .destructor = (KlAstDelete)stmtmatch_destroy, .kind = KLAST_STMT_MATCH };
static const KlAstInfo stmtlocaldef_vfunc = { .destructor = (KlAstDelete)stmtlocalfunc_destroy, .kind = KLAST_STMT_LOCALFUNC };
static const KlAstInfo stmtassign_vfunc = { .destructor = (KlAstDelete)stmtassign_destroy, .kind = KLAST_STMT_ASSIGN };
static const KlAstInfo stmtexpr_vfunc = { .destructor = (KlAstDelete)stmtexpr_destroy, .kind = KLAST_STMT_EXPR };
static const KlAstInfo stmtif_vfunc = { .destructor = (KlAstDelete)stmtif_destroy, .kind = KLAST_STMT_IF };
static const KlAstInfo stmtvfor_vfunc = { .destructor = (KlAstDelete)stmtvfor_destroy, .kind = KLAST_STMT_VFOR };
static const KlAstInfo stmtifor_vfunc = { .destructor = (KlAstDelete)stmtifor_destroy, .kind = KLAST_STMT_IFOR };
static const KlAstInfo stmtgfor_vfunc = { .destructor = (KlAstDelete)stmtgfor_destroy, .kind = KLAST_STMT_GFOR };
static const KlAstInfo stmtwhile_vfunc = { .destructor = (KlAstDelete)stmtwhile_destroy, .kind = KLAST_STMT_WHILE };
static const KlAstInfo stmtlist_vfunc = { .destructor = (KlAstDelete)stmtlist_destroy, .kind = KLAST_STMT_BLOCK };
static const KlAstInfo stmtrepeat_vfunc = { .destructor = (KlAstDelete)stmtrepeat_destroy, .kind = KLAST_STMT_REPEAT };
static const KlAstInfo stmtreturn_vfunc = { .destructor = (KlAstDelete)stmtreturn_destroy, .kind = KLAST_STMT_RETURN };
static const KlAstInfo stmtbreak_vfunc = { .destructor = (KlAstDelete)stmtbreak_destroy, .kind = KLAST_STMT_BREAK };
static const KlAstInfo stmtcontinue_vfunc = { .destructor = (KlAstDelete)stmtcontinue_destroy, .kind = KLAST_STMT_CONTINUE };


KlAstStmtLet* klast_stmtlet_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtLet* stmtlet = klast_alloc(KlAstStmtLet);
  if (kl_unlikely(!stmtlet)) {
    klast_delete(lvals);
    klast_delete(rvals);
    return NULL;
  }
  stmtlet->lvals = lvals;
  stmtlet->rvals = rvals;
  klast_setposition(stmtlet, begin, end);
  klast_init(stmtlet, &stmtlet_vfunc);
  return stmtlet;
}

KlAstStmtMethod* klast_stmtmethod_create(KlAstDot* lval, KlAstExpr* rval, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtMethod* stmtmethod = klast_alloc(KlAstStmtMethod);
  if (kl_unlikely(!stmtmethod)) {
    klast_delete(lval);
    klast_delete(rval);
    return NULL;
  }
  stmtmethod->lval = lval;
  stmtmethod->rval = rval;
  klast_setposition(stmtmethod, begin, end);
  klast_init(stmtmethod, &stmtmethod_vfunc);
  return stmtmethod;
}

KlAstStmtMatch* klast_stmtmatch_create(KlAstExpr* matchobj, KlAstExpr** patterns, KlAstStmtList** stmtlists, size_t npattern, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtMatch* stmtmatch = klast_alloc(KlAstStmtMatch);
  if (kl_unlikely(!stmtmatch)) {
    klast_delete(matchobj);
    for (size_t i = 0; i < npattern; ++i) {
      klast_delete(patterns[i]);
      klast_delete(stmtlists[i]);
    }
    free(patterns);
    free(stmtlists);
    return NULL;
  }
  stmtmatch->matchobj = matchobj;
  stmtmatch->patterns = patterns;
  stmtmatch->stmtlists = stmtlists;
  stmtmatch->npattern = npattern;
  klast_setposition(stmtmatch, begin, end);
  klast_init(stmtmatch, &stmtmatch_vfunc);
  return stmtmatch;
}

KlAstStmtLocalDefinition* klast_stmtlocaldef_create(KlStrDesc id, KlFileOffset idbegin, KlFileOffset idend, KlAstExpr* expr, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtLocalDefinition* stmtlocaldef = klast_alloc(KlAstStmtLocalDefinition);
  if (kl_unlikely(!stmtlocaldef)) {
    klast_delete(expr);
    return NULL;
  }
  stmtlocaldef->idbegin = idbegin;
  stmtlocaldef->idend = idend;
  stmtlocaldef->id = id;
  stmtlocaldef->expr = expr;
  klast_setposition(stmtlocaldef, begin, end);
  klast_init(stmtlocaldef, &stmtlocaldef_vfunc);
  return stmtlocaldef;
}

KlAstStmtAssign* klast_stmtassign_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtAssign* stmtassign = klast_alloc(KlAstStmtAssign);
  if (kl_unlikely(!stmtassign)) {
    free(lvals);
    klast_delete(rvals);
    return NULL;
  }
  stmtassign->lvals = lvals;
  stmtassign->rvals = rvals;
  klast_setposition(stmtassign, begin, end);
  klast_init(stmtassign, &stmtassign_vfunc);
  return stmtassign;
}

KlAstStmtExpr* klast_stmtexpr_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtExpr* stmtexpr = klast_alloc(KlAstStmtExpr);
  if (kl_unlikely(!stmtexpr)) {
    klast_delete(exprlist);
    return NULL;
  }
  stmtexpr->exprlist = exprlist;
  klast_setposition(stmtexpr, begin, end);
  klast_init(stmtexpr, &stmtexpr_vfunc);
  return stmtexpr;
}

KlAstStmtIf* klast_stmtif_create(KlAstExpr* cond, KlAstStmtList* then_block, KlAstStmtList* else_block, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtIf* stmtif = klast_alloc(KlAstStmtIf);
  if (kl_unlikely(!stmtif)) {
    klast_delete(cond);
    klast_delete(then_block);
    if (else_block) klast_delete(else_block);
    return NULL;
  }
  stmtif->cond = cond;
  stmtif->then_block = then_block;
  stmtif->else_block = else_block;
  klast_setposition(stmtif, begin, end);
  klast_init(stmtif, &stmtif_vfunc);
  return stmtif;
}

KlAstStmtVFor* klast_stmtvfor_create(KlAstExprList* lvals, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtVFor* stmtvfor = klast_alloc(KlAstStmtVFor);
  if (kl_unlikely(!stmtvfor)) {
    klast_delete(block);
    klast_delete(lvals);
    return NULL;
  }
  stmtvfor->block = block;
  stmtvfor->lvals = lvals;
  klast_setposition(stmtvfor, begin, end);
  klast_init(stmtvfor, &stmtvfor_vfunc);
  return stmtvfor;
}

KlAstStmtIFor* klast_stmtifor_create(KlAstExprList* lval, KlAstExpr* ibegin, KlAstExpr* iend, KlAstExpr* istep, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtIFor* stmtifor = klast_alloc(KlAstStmtIFor);
  if (kl_unlikely(!stmtifor)) {
    klast_delete(lval);
    klast_delete(block);
    klast_delete(ibegin);
    klast_delete(iend);
    if (istep) klast_delete(istep);
    return NULL;
  }
  stmtifor->lval = lval;
  stmtifor->begin = ibegin;
  stmtifor->end = iend;
  stmtifor->step = istep;
  stmtifor->block = block;
  klast_setposition(stmtifor, begin, end);
  klast_init(stmtifor, &stmtifor_vfunc);
  return stmtifor;
}

KlAstStmtGFor* klast_stmtgfor_create(KlAstExprList* lvals, KlAstExpr* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtGFor* stmtgfor = klast_alloc(KlAstStmtGFor);
  if (kl_unlikely(!stmtgfor)) {
    klast_delete(block);
    klast_delete(expr);
    klast_delete(lvals);
    return NULL;
  }
  stmtgfor->lvals = lvals;
  stmtgfor->expr = expr;
  stmtgfor->block = block;
  klast_setposition(stmtgfor, begin, end);
  klast_init(stmtgfor, &stmtgfor_vfunc);
  return stmtgfor;
}

KlAstStmtWhile* klast_stmtwhile_create(KlAstExpr* cond, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtWhile* stmtwhile = klast_alloc(KlAstStmtWhile);
  if (kl_unlikely(!stmtwhile)) {
    klast_delete(block);
    klast_delete(cond);
    return NULL;
  }
  stmtwhile->cond = cond;
  stmtwhile->block = block;
  klast_setposition(stmtwhile, begin, end);
  klast_init(stmtwhile, &stmtwhile_vfunc);
  return stmtwhile;
}

KlAstStmtList* klast_stmtlist_create(KlAstStmt** stmts, size_t nstmt, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtList* stmtlist = klast_alloc(KlAstStmtList);
  if (kl_unlikely(!stmtlist)) {
    for (size_t i = 0; i < nstmt; ++i) {
      klast_delete(stmts[i]);
    }
    free(stmts);
    return NULL;
  }
  stmtlist->stmts = stmts;
  stmtlist->nstmt = nstmt;
  klast_setposition(stmtlist, begin, end);
  klast_init(stmtlist, &stmtlist_vfunc);
  return stmtlist;
}

KlAstStmtRepeat* klast_stmtrepeat_create(KlAstStmtList* block, KlAstExpr* cond, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtRepeat* stmtrepeat = klast_alloc(KlAstStmtRepeat);
  if (kl_unlikely(!stmtrepeat)) {
    klast_delete(block);
    klast_delete(cond);
    return NULL;
  }
  stmtrepeat->cond = cond;
  stmtrepeat->block = block;
  klast_setposition(stmtrepeat, begin, end);
  klast_init(stmtrepeat, &stmtrepeat_vfunc);
  return stmtrepeat;
}

KlAstStmtReturn* klast_stmtreturn_create(KlAstExprList* retvals, KlFileOffset begin, KlFileOffset end) {
  KlAstStmtReturn* stmtreturn = klast_alloc(KlAstStmtReturn);
  if (kl_unlikely(!stmtreturn)) {
    klast_delete(retvals);
    return NULL;
  }
  stmtreturn->retvals = retvals;
  klast_setposition(stmtreturn, begin, end);
  klast_init(stmtreturn, &stmtreturn_vfunc);
  return stmtreturn;
}

KlAstStmtBreak* klast_stmtbreak_create(KlFileOffset begin, KlFileOffset end) {
  KlAstStmtBreak* stmtbreak = klast_alloc(KlAstStmtBreak);
  if (kl_unlikely(!stmtbreak)) return NULL;
  klast_setposition(stmtbreak, begin, end);
  klast_init(stmtbreak, &stmtbreak_vfunc);
  return stmtbreak;
}

KlAstStmtContinue* klast_stmtcontinue_create(KlFileOffset begin, KlFileOffset end) {
  KlAstStmtContinue* stmtcontinue = klast_alloc(KlAstStmtContinue);
  if (kl_unlikely(!stmtcontinue)) return NULL;
  klast_setposition(stmtcontinue, begin, end);
  klast_init(stmtcontinue, &stmtcontinue_vfunc);
  return stmtcontinue;
}



static void stmtlet_destroy(KlAstStmtLet* stmtlet) {
  klast_delete(stmtlet->rvals);
  klast_delete(stmtlet->lvals);
}

static void stmtmethod_destroy(KlAstStmtMethod* stmtmethod) {
  klast_delete(stmtmethod->lval);
  klast_delete(stmtmethod->rval);
}

static void stmtmatch_destroy(KlAstStmtMatch* stmtmatch) {
  klast_delete(stmtmatch->matchobj);
  KlAstStmtList** stmtlists = stmtmatch->stmtlists;
  KlAstExpr** patterns = stmtmatch->patterns;
  size_t npattern = stmtmatch->npattern;
  for (size_t i = 0; i < npattern; ++i) {
    klast_delete(patterns[i]);
    klast_delete(stmtlists[i]);
  }
  free(patterns);
  free(stmtlists);
}

static void stmtlocalfunc_destroy(KlAstStmtLocalDefinition* stmtlocalfunc) {
  klast_delete(stmtlocalfunc->expr);
}

static void stmtassign_destroy(KlAstStmtAssign* stmtassign) {
  klast_delete(stmtassign->lvals);
  klast_delete(stmtassign->rvals);
}

static void stmtexpr_destroy(KlAstStmtExpr* stmtexpr) {
  klast_delete(stmtexpr->exprlist);
}

static void stmtif_destroy(KlAstStmtIf* stmtif) {
  klast_delete(stmtif->cond);
  klast_delete(stmtif->then_block);
  if (stmtif->else_block)
    klast_delete(stmtif->else_block);
}

static void stmtvfor_destroy(KlAstStmtVFor* stmtvfor) {
  klast_delete(stmtvfor->block);
  klast_delete(stmtvfor->lvals);
}

static void stmtifor_destroy(KlAstStmtIFor* stmtifor) {
  klast_delete(stmtifor->lval);
  klast_delete(stmtifor->begin);
  klast_delete(stmtifor->end);
  if (stmtifor->step) klast_delete(stmtifor->step);
  klast_delete(stmtifor->block);
}

static void stmtgfor_destroy(KlAstStmtGFor* stmtgfor) {
  klast_delete(stmtgfor->block);
  klast_delete(stmtgfor->expr);
  klast_delete(stmtgfor->lvals);
}

static void stmtwhile_destroy(KlAstStmtWhile* stmtwhile) {
  klast_delete(stmtwhile->cond);
  klast_delete(stmtwhile->block);
}

static void stmtlist_destroy(KlAstStmtList* stmtblock) {
  size_t nstmt = stmtblock->nstmt;
  KlAstStmt** stmts = stmtblock->stmts;
  for (size_t i = 0; i < nstmt; ++i)
    klast_delete(stmts[i]);
  free(stmts);
}

static void stmtrepeat_destroy(KlAstStmtRepeat* stmtrepeat) {
  klast_delete(stmtrepeat->block);
  klast_delete(stmtrepeat->cond);
}

static void stmtreturn_destroy(KlAstStmtReturn* stmtreturn) {
  klast_delete(stmtreturn->retvals);
}

static void stmtbreak_destroy(KlAstStmtBreak* stmtbreak) {
  kl_unused(stmtbreak);
}

static void stmtcontinue_destroy(KlAstStmtContinue* stmtcontinue) {
  kl_unused(stmtcontinue);
}


bool klast_mustreturn(KlAstStmtList* stmtlist) {
  if (stmtlist->nstmt == 0) return false;
  KlAstStmt* laststmt = stmtlist->stmts[stmtlist->nstmt - 1];
  if (klast_kind(laststmt) == KLAST_STMT_RETURN)
    return true;
  if (klast_kind(laststmt) == KLAST_STMT_IF) {
    KlAstStmtIf* stmtif = klcast(KlAstStmtIf*, laststmt);
    return klast_mustreturn(stmtif->then_block) &&
           (!stmtif->else_block || klast_mustreturn(stmtif->else_block));
  }
  return false;
}

