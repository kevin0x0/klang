#include "klang/include/cst/klcst_stmt.h"
#include <stdlib.h>


static void klcst_stmtlet_delete(KlCstStmtLet* stmtlet);
static void klcst_stmtassign_delete(KlCstStmtAssign* stmtassign);
static void klcst_stmtexpr_delete(KlCstStmtExpr* stmtexpr);
static void klcst_stmtif_delete(KlCstStmtIf* stmtif);
static void klcst_stmtvfor_delete(KlCstStmtVFor* stmtvfor);
static void klcst_stmtifor_delete(KlCstStmtIFor* stmtifor);
static void klcst_stmtgfor_delete(KlCstStmtGFor* stmtgfor);
static void klcst_stmtcfor_delete(KlCstStmtCFor* stmtcfor);
static void klcst_stmtwhile_delete(KlCstStmtWhile* stmtwhile);
static void klcst_stmtlist_delete(KlCstStmtList* stmtblock);
static void klcst_stmtrepeat_delete(KlCstStmtRepeat* stmtrepeat);
static void klcst_stmtreturn_delete(KlCstStmtReturn* stmtreturn);
static void klcst_stmtbreak_delete(KlCstStmtBreak* stmtbreak);
static void klcst_stmtcontinue_delete(KlCstStmtContinue* stmtcontinue);

static KlCstVirtualFunc klcst_stmtlet_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtlet_delete };
static KlCstVirtualFunc klcst_stmtassign_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtassign_delete };
static KlCstVirtualFunc klcst_stmtexpr_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtexpr_delete };
static KlCstVirtualFunc klcst_stmtif_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtif_delete };
static KlCstVirtualFunc klcst_stmtvfor_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtvfor_delete };
static KlCstVirtualFunc klcst_stmtifor_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtifor_delete };
static KlCstVirtualFunc klcst_stmtgfor_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtgfor_delete };
static KlCstVirtualFunc klcst_stmtcfor_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtcfor_delete };
static KlCstVirtualFunc klcst_stmtwhile_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtwhile_delete };
static KlCstVirtualFunc klcst_stmtblock_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtlist_delete };
static KlCstVirtualFunc klcst_stmtrepeat_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtrepeat_delete };
static KlCstVirtualFunc klcst_stmtreturn_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtreturn_delete };
static KlCstVirtualFunc klcst_stmtbreak_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtbreak_delete };
static KlCstVirtualFunc klcst_stmtcontinue_vfunc = { .cstdelete = (KlCstDelete)klcst_stmtcontinue_delete };



KlCstStmtLet* klcst_stmtlet_create(void) {
  KlCstStmtLet* stmtlet = (KlCstStmtLet*)malloc(sizeof (KlCstStmtLet));
  if (kl_unlikely(!stmtlet)) return NULL;
  klcst_init(klcast(KlCst*, stmtlet), KLCST_STMT_LET, &klcst_stmtlet_vfunc);
  return stmtlet;
}

KlCstStmtAssign* klcst_stmtassign_create(void) {
  KlCstStmtAssign* stmtassign = (KlCstStmtAssign*)malloc(sizeof (KlCstStmtAssign));
  if (kl_unlikely(!stmtassign)) return NULL;
  klcst_init(klcast(KlCst*, stmtassign), KLCST_STMT_ASSIGN, &klcst_stmtassign_vfunc);
  return stmtassign;
}

KlCstStmtExpr* klcst_stmtexpr_create(void) {
  KlCstStmtExpr* stmtexpr = (KlCstStmtExpr*)malloc(sizeof (KlCstStmtExpr));
  if (kl_unlikely(!stmtexpr)) return NULL;
  klcst_init(klcast(KlCst*, stmtexpr), KLCST_STMT_EXPR, &klcst_stmtexpr_vfunc);
  return stmtexpr;
} 

KlCstStmtIf* klcst_stmtif_create(void) {
  KlCstStmtIf* stmtif = (KlCstStmtIf*)malloc(sizeof (KlCstStmtIf));
  if (kl_unlikely(!stmtif)) return NULL;
  klcst_init(klcast(KlCst*, stmtif), KLCST_STMT_IF, &klcst_stmtif_vfunc);
  return stmtif;
}

KlCstStmtVFor* klcst_stmtvfor_create(void) {
  KlCstStmtVFor* stmtvfor = (KlCstStmtVFor*)malloc(sizeof (KlCstStmtVFor));
  if (kl_unlikely(!stmtvfor)) return NULL;
  klcst_init(klcast(KlCst*, stmtvfor), KLCST_STMT_VFOR, &klcst_stmtvfor_vfunc);
  return stmtvfor;
}

KlCstStmtIFor* klcst_stmtifor_create(void) {
  KlCstStmtIFor* stmtifor = (KlCstStmtIFor*)malloc(sizeof (KlCstStmtIFor));
  if (kl_unlikely(!stmtifor)) return NULL;
  klcst_init(klcast(KlCst*, stmtifor), KLCST_STMT_IFOR, &klcst_stmtifor_vfunc);
  return stmtifor;
}

KlCstStmtGFor* klcst_stmtgfor_create(void) {
  KlCstStmtGFor* stmtgfor = (KlCstStmtGFor*)malloc(sizeof (KlCstStmtGFor));
  if (kl_unlikely(!stmtgfor)) return NULL;
  klcst_init(klcast(KlCst*, stmtgfor), KLCST_STMT_GFOR, &klcst_stmtgfor_vfunc);
  return stmtgfor;
}

KlCstStmtCFor* klcst_stmtcfor_create(void) {
  KlCstStmtCFor* stmtcfor = (KlCstStmtCFor*)malloc(sizeof (KlCstStmtCFor));
  if (kl_unlikely(!stmtcfor)) return NULL;
  klcst_init(klcast(KlCst*, stmtcfor), KLCST_STMT_CFOR, &klcst_stmtcfor_vfunc);
  return stmtcfor;
}

KlCstStmtWhile* klcst_stmtwhile_create(void) {
  KlCstStmtWhile* stmtwhile = (KlCstStmtWhile*)malloc(sizeof (KlCstStmtWhile));
  if (kl_unlikely(!stmtwhile)) return NULL;
  klcst_init(klcast(KlCst*, stmtwhile), KLCST_STMT_WHILE, &klcst_stmtwhile_vfunc);
  return stmtwhile;
}

KlCstStmtList* klcst_stmtlist_create(void) {
  KlCstStmtList* stmtblock = (KlCstStmtList*)malloc(sizeof (KlCstStmtList));
  if (kl_unlikely(!stmtblock)) return NULL;
  klcst_init(klcast(KlCst*, stmtblock), KLCST_STMT_BLOCK, &klcst_stmtblock_vfunc);
  return stmtblock;
}

KlCstStmtRepeat* klcst_stmtrepeat_create(void) {
  KlCstStmtRepeat* stmtrepeat = (KlCstStmtRepeat*)malloc(sizeof (KlCstStmtRepeat));
  if (kl_unlikely(!stmtrepeat)) return NULL;
  klcst_init(klcast(KlCst*, stmtrepeat), KLCST_STMT_REPEAT, &klcst_stmtrepeat_vfunc);
  return stmtrepeat;
}

KlCstStmtReturn* klcst_stmtreturn_create(void) {
  KlCstStmtReturn* stmtreturn = (KlCstStmtReturn*)malloc(sizeof (KlCstStmtReturn));
  if (kl_unlikely(!stmtreturn)) return NULL;
  klcst_init(klcast(KlCst*, stmtreturn), KLCST_STMT_RETURN, &klcst_stmtreturn_vfunc);
  return stmtreturn;
}

KlCstStmtBreak* klcst_stmtbreak_create(void) {
  KlCstStmtBreak* stmtbreak = (KlCstStmtBreak*)malloc(sizeof (KlCstStmtBreak));
  if (kl_unlikely(!stmtbreak)) return NULL;
  klcst_init(klcast(KlCst*, stmtbreak), KLCST_STMT_BREAK, &klcst_stmtbreak_vfunc);
  return stmtbreak;
}

KlCstStmtContinue* klcst_stmtcontinue_create(void) {
  KlCstStmtContinue* stmtcontinue = (KlCstStmtContinue*)malloc(sizeof (KlCstStmtContinue));
  if (kl_unlikely(!stmtcontinue)) return NULL;
  klcst_init(klcast(KlCst*, stmtcontinue), KLCST_STMT_CONTINUE, &klcst_stmtcontinue_vfunc);
  return stmtcontinue;
}


static void klcst_stmtlet_delete(KlCstStmtLet* stmtlet) {
  free(stmtlet->lvals);
  klcst_delete(stmtlet->rvals);
  free(stmtlet);
}

static void klcst_stmtassign_delete(KlCstStmtAssign* stmtassign) {
  klcst_delete(stmtassign->lvals);
  klcst_delete(stmtassign->rvals);
  free(stmtassign);
}

static void klcst_stmtexpr_delete(KlCstStmtExpr* stmtexpr) {
  klcst_delete(stmtexpr->expr);
  free(stmtexpr);
}

static void klcst_stmtif_delete(KlCstStmtIf* stmtif) {
  klcst_delete(stmtif->cond);
  klcst_delete(stmtif->if_block);
  if (stmtif->else_block)
    klcst_delete(stmtif->else_block);
  free(stmtif);
}

static void klcst_stmtvfor_delete(KlCstStmtVFor* stmtvfor) {
  free(stmtvfor->ids);
  klcst_delete(stmtvfor->block);
  free(stmtvfor);
}

static void klcst_stmtifor_delete(KlCstStmtIFor* stmtifor) {
  klcst_delete(stmtifor->begin);
  klcst_delete(stmtifor->end);
  if (stmtifor->step)
    klcst_delete(stmtifor->step);
  klcst_delete(stmtifor->block);
  free(stmtifor);
}

static void klcst_stmtgfor_delete(KlCstStmtGFor* stmtgfor) {
  klcst_delete(stmtgfor->expr);
  free(stmtgfor->ids);
  klcst_delete(stmtgfor->block);
  free(stmtgfor);
}

static void klcst_stmtcfor_delete(KlCstStmtCFor* stmtcfor) {
  if (stmtcfor->init)
    klcst_delete(stmtcfor->init);
  if (stmtcfor->cond)
    klcst_delete(stmtcfor->cond);
  if (stmtcfor->post)
    klcst_delete(stmtcfor->post);
  klcst_delete(stmtcfor->block);
  free(stmtcfor);
}

static void klcst_stmtwhile_delete(KlCstStmtWhile* stmtwhile) {
  klcst_delete(stmtwhile->cond);
  klcst_delete(stmtwhile->block);
  free(stmtwhile);
}

static void klcst_stmtlist_delete(KlCstStmtList* stmtblock) {
  size_t nstmt = stmtblock->nstmt;
  KlCst** stmts = stmtblock->stmts;
  for (size_t i = 0; i < nstmt; ++i)
    klcst_delete(stmts[i]);
  free(stmts);
  free(stmtblock);
}

static void klcst_stmtrepeat_delete(KlCstStmtRepeat* stmtrepeat) {
  klcst_delete(stmtrepeat->block);
  klcst_delete(stmtrepeat->cond);
  free(stmtrepeat);
}

static void klcst_stmtreturn_delete(KlCstStmtReturn* stmtreturn) {
  if (stmtreturn->retval)
    klcst_delete(stmtreturn->retval);
  free(stmtreturn);
}

static void klcst_stmtbreak_delete(KlCstStmtBreak* stmtbreak) {
  free(stmtbreak);
}

static void klcst_stmtcontinue_delete(KlCstStmtContinue* stmtcontinue) {
  free(stmtcontinue);
}

