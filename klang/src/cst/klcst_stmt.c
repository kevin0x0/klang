#include "klang/include/cst/klcst_stmt.h"
#include "klang/include/cst/klcst.h"
#include <stdlib.h>


static void klcst_stmtlet_destroy(KlCstStmtLet* stmtlet);
static void klcst_stmtassign_destroy(KlCstStmtAssign* stmtassign);
static void klcst_stmtexpr_destroy(KlCstStmtExpr* stmtexpr);
static void klcst_stmtif_destroy(KlCstStmtIf* stmtif);
static void klcst_stmtvfor_destroy(KlCstStmtVFor* stmtvfor);
static void klcst_stmtifor_destroy(KlCstStmtIFor* stmtifor);
static void klcst_stmtgfor_destroy(KlCstStmtGFor* stmtgfor);
static void klcst_stmtcfor_destroy(KlCstStmtCFor* stmtcfor);
static void klcst_stmtwhile_destroy(KlCstStmtWhile* stmtwhile);
static void klcst_stmtlist_destroy(KlCstStmtList* stmtblock);
static void klcst_stmtrepeat_destroy(KlCstStmtRepeat* stmtrepeat);
static void klcst_stmtreturn_destroy(KlCstStmtReturn* stmtreturn);
static void klcst_stmtbreak_destroy(KlCstStmtBreak* stmtbreak);
static void klcst_stmtcontinue_destroy(KlCstStmtContinue* stmtcontinue);

static KlCstInfo klcst_stmtlet_vfunc = { .destructor = (KlCstDelete)klcst_stmtlet_destroy, .kind = KLCST_STMT_LET };
static KlCstInfo klcst_stmtassign_vfunc = { .destructor = (KlCstDelete)klcst_stmtassign_destroy, .kind = KLCST_STMT_ASSIGN };
static KlCstInfo klcst_stmtexpr_vfunc = { .destructor = (KlCstDelete)klcst_stmtexpr_destroy, .kind = KLCST_STMT_EXPR };
static KlCstInfo klcst_stmtif_vfunc = { .destructor = (KlCstDelete)klcst_stmtif_destroy, .kind = KLCST_STMT_IF };
static KlCstInfo klcst_stmtvfor_vfunc = { .destructor = (KlCstDelete)klcst_stmtvfor_destroy, .kind = KLCST_STMT_VFOR };
static KlCstInfo klcst_stmtifor_vfunc = { .destructor = (KlCstDelete)klcst_stmtifor_destroy, .kind = KLCST_STMT_IFOR };
static KlCstInfo klcst_stmtgfor_vfunc = { .destructor = (KlCstDelete)klcst_stmtgfor_destroy, .kind = KLCST_STMT_GFOR };
static KlCstInfo klcst_stmtcfor_vfunc = { .destructor = (KlCstDelete)klcst_stmtcfor_destroy, .kind = KLCST_STMT_CFOR };
static KlCstInfo klcst_stmtwhile_vfunc = { .destructor = (KlCstDelete)klcst_stmtwhile_destroy, .kind = KLCST_STMT_WHILE };
static KlCstInfo klcst_stmtlist_vfunc = { .destructor = (KlCstDelete)klcst_stmtlist_destroy, .kind = KLCST_STMT_BLOCK };
static KlCstInfo klcst_stmtrepeat_vfunc = { .destructor = (KlCstDelete)klcst_stmtrepeat_destroy, .kind = KLCST_STMT_REPEAT };
static KlCstInfo klcst_stmtreturn_vfunc = { .destructor = (KlCstDelete)klcst_stmtreturn_destroy, .kind = KLCST_STMT_RETURN };
static KlCstInfo klcst_stmtbreak_vfunc = { .destructor = (KlCstDelete)klcst_stmtbreak_destroy, .kind = KLCST_STMT_BREAK };
static KlCstInfo klcst_stmtcontinue_vfunc = { .destructor = (KlCstDelete)klcst_stmtcontinue_destroy, .kind = KLCST_STMT_CONTINUE };


KlCstStmtLet* klcst_stmtlet_create(KlStrDesc* lvals, size_t nlval, KlCst* rvals, KlFilePos begin, KlFilePos end) {
  KlCstStmtLet* stmtlet = klcst_alloc(KlCstStmtLet);
  if (kl_unlikely(!stmtlet)) {
    free(lvals);
    klcst_delete_raw(rvals);
    return NULL;
  }
  stmtlet->lvals = lvals;
  stmtlet->nlval = nlval;
  stmtlet->rvals = rvals;
  klcst_setposition(stmtlet, begin, end);
  klcst_init(stmtlet, &klcst_stmtlet_vfunc);
  return stmtlet;
}

KlCstStmtAssign* klcst_stmtassign_create(KlCst* lvals, KlCst* rvals, KlFilePos begin, KlFilePos end) {
  KlCstStmtAssign* stmtassign = klcst_alloc(KlCstStmtAssign);
  if (kl_unlikely(!stmtassign)) {
    free(lvals);
    klcst_delete_raw(rvals);
    return NULL;
  }
  stmtassign->lvals = lvals;
  stmtassign->rvals = rvals;
  klcst_setposition(stmtassign, begin, end);
  klcst_init(stmtassign, &klcst_stmtassign_vfunc);
  return stmtassign;
}

KlCstStmtExpr* klcst_stmtexpr_create(KlCst* expr, KlFilePos begin, KlFilePos end) {
  KlCstStmtExpr* stmtexpr = klcst_alloc(KlCstStmtExpr);
  if (kl_unlikely(!stmtexpr)) {
    klcst_delete_raw(expr);
    return NULL;
  }
  stmtexpr->expr = expr;
  klcst_setposition(stmtexpr, begin, end);
  klcst_init(stmtexpr, &klcst_stmtexpr_vfunc);
  return stmtexpr;
}

KlCstStmtIf* klcst_stmtif_create(KlCst* cond, KlCst* if_block, KlCst* else_block, KlFilePos begin, KlFilePos end) {
  KlCstStmtIf* stmtif = klcst_alloc(KlCstStmtIf);
  if (kl_unlikely(!stmtif)) {
    klcst_delete_raw(cond);
    klcst_delete_raw(if_block);
    klcst_delete_raw(else_block);
    return NULL;
  }
  stmtif->cond = cond;
  stmtif->if_block = if_block;
  stmtif->else_block = else_block;
  klcst_setposition(stmtif, begin, end);
  klcst_init(stmtif, &klcst_stmtif_vfunc);
  return stmtif;
}

KlCstStmtVFor* klcst_stmtvfor_create(KlStrDesc* ids, size_t nid, KlCst* block, KlFilePos begin, KlFilePos end) {
  KlCstStmtVFor* stmtvfor = klcst_alloc(KlCstStmtVFor);
  if (kl_unlikely(!stmtvfor)) {
    klcst_delete_raw(block);
    free(ids);
    return NULL;
  }
  stmtvfor->block = block;
  stmtvfor->ids = ids;
  stmtvfor->nid = nid;
  klcst_setposition(stmtvfor, begin, end);
  klcst_init(stmtvfor, &klcst_stmtvfor_vfunc);
  return stmtvfor;
}

KlCstStmtIFor* klcst_stmtifor_create(KlStrDesc id, KlCst* ibegin, KlCst* iend, KlCst* istep, KlCst* block, KlFilePos begin, KlFilePos end) {
  KlCstStmtIFor* stmtifor = klcst_alloc(KlCstStmtIFor);
  if (kl_unlikely(!stmtifor)) {
    klcst_delete_raw(block);
    klcst_delete_raw(ibegin);
    klcst_delete_raw(iend);
    klcst_delete_raw(istep);
    return NULL;
  }
  stmtifor->id = id;
  stmtifor->begin = ibegin;
  stmtifor->end = iend;
  stmtifor->step = istep;
  stmtifor->block = block;
  klcst_setposition(stmtifor, begin, end);
  klcst_init(stmtifor, &klcst_stmtifor_vfunc);
  return stmtifor;
}

KlCstStmtGFor* klcst_stmtgfor_create(KlStrDesc* ids, size_t nid, KlCst* expr, KlCst* block, KlFilePos begin, KlFilePos end) {
  KlCstStmtGFor* stmtgfor = klcst_alloc(KlCstStmtGFor);
  if (kl_unlikely(!stmtgfor)) {
    klcst_delete_raw(block);
    klcst_delete_raw(expr);
    free(ids);
    return NULL;
  }
  stmtgfor->ids = ids;
  stmtgfor->nid = nid;
  stmtgfor->expr = expr;
  stmtgfor->block = block;
  klcst_setposition(stmtgfor, begin, end);
  klcst_init(stmtgfor, &klcst_stmtgfor_vfunc);
  return stmtgfor;
}

KlCstStmtCFor* klcst_stmtcfor_create(KlCst* init, KlCst* cond, KlCst* post, KlCst* block, KlFilePos begin, KlFilePos end) {
  KlCstStmtCFor* stmtcfor = klcst_alloc(KlCstStmtCFor);
  if (kl_unlikely(!stmtcfor)) {
    klcst_delete_raw(block);
    if (init) klcst_delete_raw(init);
    if (cond) klcst_delete_raw(cond);
    if (post) klcst_delete_raw(post);
    return NULL;
  }
  stmtcfor->init = init;
  stmtcfor->cond = cond;
  stmtcfor->post = post;
  stmtcfor->block = block;
  klcst_setposition(stmtcfor, begin, end);
  klcst_init(stmtcfor, &klcst_stmtcfor_vfunc);
  return stmtcfor;
}

KlCstStmtWhile* klcst_stmtwhile_create(KlCst* cond, KlCst* block, KlFilePos begin, KlFilePos end) {
  KlCstStmtWhile* stmtwhile = klcst_alloc(KlCstStmtWhile);
  if (kl_unlikely(!stmtwhile)) {
    klcst_delete_raw(block);
    klcst_delete_raw(cond);
    return NULL;
  }
  stmtwhile->cond = cond;
  stmtwhile->block = block;
  klcst_setposition(stmtwhile, begin, end);
  klcst_init(stmtwhile, &klcst_stmtwhile_vfunc);
  return stmtwhile;
}

KlCstStmtList* klcst_stmtlist_create(KlCst** stmts, size_t nstmt, KlFilePos begin, KlFilePos end) {
  KlCstStmtList* stmtlist = klcst_alloc(KlCstStmtList);
  if (kl_unlikely(!stmtlist)) {
    for (size_t i = 0; i < nstmt; ++i) {
      klcst_delete_raw(stmts[i]);
    }
    free(stmts);
    return NULL;
  }
  stmtlist->stmts = stmts;
  stmtlist->nstmt = nstmt;
  klcst_setposition(stmtlist, begin, end);
  klcst_init(stmtlist, &klcst_stmtlist_vfunc);
  return stmtlist;
}

KlCstStmtRepeat* klcst_stmtrepeat_create(KlCst* block, KlCst* cond, KlFilePos begin, KlFilePos end) {
  KlCstStmtRepeat* stmtrepeat = klcst_alloc(KlCstStmtRepeat);
  if (kl_unlikely(!stmtrepeat)) {
    klcst_delete_raw(block);
    klcst_delete_raw(cond);
    return NULL;
  }
  stmtrepeat->cond = cond;
  stmtrepeat->block = block;
  klcst_setposition(stmtrepeat, begin, end);
  klcst_init(stmtrepeat, &klcst_stmtrepeat_vfunc);
  return stmtrepeat;
}

KlCstStmtReturn* klcst_stmtreturn_create(KlCst* retval, KlFilePos begin, KlFilePos end) {
  KlCstStmtReturn* stmtreturn = klcst_alloc(KlCstStmtReturn);
  if (kl_unlikely(!stmtreturn)) {
    klcst_delete_raw(retval);
    return NULL;
  }
  stmtreturn->retval = retval;
  klcst_setposition(stmtreturn, begin, end);
  klcst_init(stmtreturn, &klcst_stmtreturn_vfunc);
  return stmtreturn;
}

KlCstStmtBreak* klcst_stmtbreak_create(KlFilePos begin, KlFilePos end) {
  KlCstStmtBreak* stmtbreak = klcst_alloc(KlCstStmtBreak);
  if (kl_unlikely(!stmtbreak)) return NULL;
  klcst_setposition(stmtbreak, begin, end);
  klcst_init(stmtbreak, &klcst_stmtbreak_vfunc);
  return stmtbreak;
}

KlCstStmtContinue* klcst_stmtcontinue_create(KlFilePos begin, KlFilePos end) {
  KlCstStmtContinue* stmtcontinue = klcst_alloc(KlCstStmtContinue);
  if (kl_unlikely(!stmtcontinue)) return NULL;
  klcst_setposition(stmtcontinue, begin, end);
  klcst_init(stmtcontinue, &klcst_stmtcontinue_vfunc);
  return stmtcontinue;
}



static void klcst_stmtlet_destroy(KlCstStmtLet* stmtlet) {
  klcst_delete_raw(stmtlet->rvals);
  free(stmtlet->lvals);
}

static void klcst_stmtassign_destroy(KlCstStmtAssign* stmtassign) {
  klcst_delete_raw(stmtassign->lvals);
  klcst_delete_raw(stmtassign->rvals);
}

static void klcst_stmtexpr_destroy(KlCstStmtExpr* stmtexpr) {
  klcst_delete_raw(stmtexpr->expr);
}

static void klcst_stmtif_destroy(KlCstStmtIf* stmtif) {
  klcst_delete_raw(stmtif->cond);
  klcst_delete_raw(stmtif->if_block);
  if (stmtif->else_block)
    klcst_delete_raw(stmtif->else_block);
}

static void klcst_stmtvfor_destroy(KlCstStmtVFor* stmtvfor) {
  klcst_delete_raw(stmtvfor->block);
  free(stmtvfor->ids);
}

static void klcst_stmtifor_destroy(KlCstStmtIFor* stmtifor) {
  klcst_delete_raw(stmtifor->begin);
  klcst_delete_raw(stmtifor->end);
  if (stmtifor->step) klcst_delete_raw(stmtifor->step);
  klcst_delete_raw(stmtifor->block);
}

static void klcst_stmtgfor_destroy(KlCstStmtGFor* stmtgfor) {
  klcst_delete_raw(stmtgfor->block);
  klcst_delete_raw(stmtgfor->expr);
  free(stmtgfor->ids);
}

static void klcst_stmtcfor_destroy(KlCstStmtCFor* stmtcfor) {
  if (stmtcfor->init) klcst_delete_raw(stmtcfor->init);
  if (stmtcfor->cond) klcst_delete_raw(stmtcfor->cond);
  if (stmtcfor->post) klcst_delete_raw(stmtcfor->post);
  klcst_delete_raw(stmtcfor->block);
}

static void klcst_stmtwhile_destroy(KlCstStmtWhile* stmtwhile) {
  klcst_delete_raw(stmtwhile->cond);
  klcst_delete_raw(stmtwhile->block);
}

static void klcst_stmtlist_destroy(KlCstStmtList* stmtblock) {
  size_t nstmt = stmtblock->nstmt;
  KlCst** stmts = stmtblock->stmts;
  for (size_t i = 0; i < nstmt; ++i)
    klcst_delete_raw(stmts[i]);
  free(stmts);
}

static void klcst_stmtrepeat_destroy(KlCstStmtRepeat* stmtrepeat) {
  klcst_delete_raw(stmtrepeat->block);
  klcst_delete_raw(stmtrepeat->cond);
}

static void klcst_stmtreturn_destroy(KlCstStmtReturn* stmtreturn) {
  klcst_delete_raw(stmtreturn->retval);
}

static void klcst_stmtbreak_destroy(KlCstStmtBreak* stmtbreak) {
  (void)stmtbreak;
}

static void klcst_stmtcontinue_destroy(KlCstStmtContinue* stmtcontinue) {
  (void)stmtcontinue;
}

