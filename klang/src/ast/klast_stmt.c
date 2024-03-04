#include "klang/include/ast/klast_stmt.h"
#include <stdlib.h>


static void klast_stmtlet_delete(KlAstStmtLet* stmtlet);
static void klast_stmtassign_delete(KlAstStmtAssign* stmtassign);
static void klast_stmtexpr_delete(KlAstStmtExpr* stmtexpr);
static void klast_stmtif_delete(KlAstStmtIf* stmtif);
static void klast_stmtvfor_delete(KlAstStmtVFor* stmtvfor);
static void klast_stmtifor_delete(KlAstStmtIFor* stmtifor);
static void klast_stmtgfor_delete(KlAstStmtGFor* stmtgfor);
static void klast_stmtcfor_delete(KlAstStmtCFor* stmtcfor);
static void klast_stmtwhile_delete(KlAstStmtWhile* stmtwhile);
static void klast_stmtblock_delete(KlAstStmtBlock* stmtblock);
static void klast_stmtrepeat_delete(KlAstStmtRepeat* stmtrepeat);
static void klast_stmtreturn_delete(KlAstStmtReturn* stmtreturn);
static void klast_stmtbreak_delete(KlAstStmtBreak* stmtbreak);
static void klast_stmtcontinue_delete(KlAstStmtContinue* stmtcontinue);

static KlAstVirtualFunc klast_stmtlet_vfunc = { .astdelete = (KlAstDelete)klast_stmtlet_delete };
static KlAstVirtualFunc klast_stmtassign_vfunc = { .astdelete = (KlAstDelete)klast_stmtassign_delete };
static KlAstVirtualFunc klast_stmtexpr_vfunc = { .astdelete = (KlAstDelete)klast_stmtexpr_delete };
static KlAstVirtualFunc klast_stmtif_vfunc = { .astdelete = (KlAstDelete)klast_stmtif_delete };
static KlAstVirtualFunc klast_stmtvfor_vfunc = { .astdelete = (KlAstDelete)klast_stmtvfor_delete };
static KlAstVirtualFunc klast_stmtifor_vfunc = { .astdelete = (KlAstDelete)klast_stmtifor_delete };
static KlAstVirtualFunc klast_stmtgfor_vfunc = { .astdelete = (KlAstDelete)klast_stmtgfor_delete };
static KlAstVirtualFunc klast_stmtcfor_vfunc = { .astdelete = (KlAstDelete)klast_stmtcfor_delete };
static KlAstVirtualFunc klast_stmtwhile_vfunc = { .astdelete = (KlAstDelete)klast_stmtwhile_delete };
static KlAstVirtualFunc klast_stmtblock_vfunc = { .astdelete = (KlAstDelete)klast_stmtblock_delete };
static KlAstVirtualFunc klast_stmtrepeat_vfunc = { .astdelete = (KlAstDelete)klast_stmtrepeat_delete };
static KlAstVirtualFunc klast_stmtreturn_vfunc = { .astdelete = (KlAstDelete)klast_stmtreturn_delete };
static KlAstVirtualFunc klast_stmtbreak_vfunc = { .astdelete = (KlAstDelete)klast_stmtbreak_delete };
static KlAstVirtualFunc klast_stmtcontinue_vfunc = { .astdelete = (KlAstDelete)klast_stmtcontinue_delete };



KlAstStmtLet* klast_stmtlet_create(void) {
  KlAstStmtLet* stmtlet = (KlAstStmtLet*)malloc(sizeof (KlAstStmtLet));
  if (kl_unlikely(!stmtlet)) return NULL;
  klast_init(klcast(KlAst*, stmtlet), KLAST_STMT_LET, &klast_stmtlet_vfunc);
  return stmtlet;
}

KlAstStmtAssign* klast_stmtassign_create(void) {
  KlAstStmtAssign* stmtassign = (KlAstStmtAssign*)malloc(sizeof (KlAstStmtAssign));
  if (kl_unlikely(!stmtassign)) return NULL;
  klast_init(klcast(KlAst*, stmtassign), KLAST_STMT_ASSIGN, &klast_stmtassign_vfunc);
  return stmtassign;
}

KlAstStmtExpr* klast_stmtexpr_create(void) {
  KlAstStmtExpr* stmtexpr = (KlAstStmtExpr*)malloc(sizeof (KlAstStmtExpr));
  if (kl_unlikely(!stmtexpr)) return NULL;
  klast_init(klcast(KlAst*, stmtexpr), KLAST_STMT_EXPR, &klast_stmtexpr_vfunc);
  return stmtexpr;
} 

KlAstStmtIf* klast_stmtif_create(void) {
  KlAstStmtIf* stmtif = (KlAstStmtIf*)malloc(sizeof (KlAstStmtIf));
  if (kl_unlikely(!stmtif)) return NULL;
  klast_init(klcast(KlAst*, stmtif), KLAST_STMT_ASSIGN, &klast_stmtif_vfunc);
  return stmtif;
}

KlAstStmtVFor* klast_stmtvfor_create(void) {
  KlAstStmtVFor* stmtvfor = (KlAstStmtVFor*)malloc(sizeof (KlAstStmtVFor));
  if (kl_unlikely(!stmtvfor)) return NULL;
  klast_init(klcast(KlAst*, stmtvfor), KLAST_STMT_ASSIGN, &klast_stmtvfor_vfunc);
  return stmtvfor;
}

KlAstStmtIFor* klast_stmtifor_create(void) {
  KlAstStmtIFor* stmtifor = (KlAstStmtIFor*)malloc(sizeof (KlAstStmtIFor));
  if (kl_unlikely(!stmtifor)) return NULL;
  klast_init(klcast(KlAst*, stmtifor), KLAST_STMT_ASSIGN, &klast_stmtifor_vfunc);
  return stmtifor;
}

KlAstStmtGFor* klast_stmtgfor_create(void) {
  KlAstStmtGFor* stmtgfor = (KlAstStmtGFor*)malloc(sizeof (KlAstStmtGFor));
  if (kl_unlikely(!stmtgfor)) return NULL;
  klast_init(klcast(KlAst*, stmtgfor), KLAST_STMT_ASSIGN, &klast_stmtgfor_vfunc);
  return stmtgfor;
}

KlAstStmtCFor* klast_stmtcfor_create(void) {
  KlAstStmtCFor* stmtcfor = (KlAstStmtCFor*)malloc(sizeof (KlAstStmtCFor));
  if (kl_unlikely(!stmtcfor)) return NULL;
  klast_init(klcast(KlAst*, stmtcfor), KLAST_STMT_ASSIGN, &klast_stmtcfor_vfunc);
  return stmtcfor;
}

KlAstStmtWhile* klast_stmtwhile_create(void) {
  KlAstStmtWhile* stmtwhile = (KlAstStmtWhile*)malloc(sizeof (KlAstStmtWhile));
  if (kl_unlikely(!stmtwhile)) return NULL;
  klast_init(klcast(KlAst*, stmtwhile), KLAST_STMT_ASSIGN, &klast_stmtwhile_vfunc);
  return stmtwhile;
}

KlAstStmtBlock* klast_stmtblock_create(void) {
  KlAstStmtBlock* stmtblock = (KlAstStmtBlock*)malloc(sizeof (KlAstStmtBlock));
  if (kl_unlikely(!stmtblock)) return NULL;
  klast_init(klcast(KlAst*, stmtblock), KLAST_STMT_ASSIGN, &klast_stmtblock_vfunc);
  return stmtblock;
}

KlAstStmtRepeat* klast_stmtrepeat_create(void) {
  KlAstStmtRepeat* stmtrepeat = (KlAstStmtRepeat*)malloc(sizeof (KlAstStmtRepeat));
  if (kl_unlikely(!stmtrepeat)) return NULL;
  klast_init(klcast(KlAst*, stmtrepeat), KLAST_STMT_REPEAT, &klast_stmtrepeat_vfunc);
  return stmtrepeat;
}

KlAstStmtReturn* klast_stmtreturn_create(void) {
  KlAstStmtReturn* stmtreturn = (KlAstStmtReturn*)malloc(sizeof (KlAstStmtReturn));
  if (kl_unlikely(!stmtreturn)) return NULL;
  klast_init(klcast(KlAst*, stmtreturn), KLAST_STMT_RETURN, &klast_stmtreturn_vfunc);
  return stmtreturn;
}

KlAstStmtBreak* klast_stmtbreak_create(void) {
  KlAstStmtBreak* stmtbreak = (KlAstStmtBreak*)malloc(sizeof (KlAstStmtBreak));
  if (kl_unlikely(!stmtbreak)) return NULL;
  klast_init(klcast(KlAst*, stmtbreak), KLAST_STMT_BREAK, &klast_stmtbreak_vfunc);
  return stmtbreak;
}

KlAstStmtContinue* klast_stmtcontinue_create(void) {
  KlAstStmtContinue* stmtcontinue = (KlAstStmtContinue*)malloc(sizeof (KlAstStmtContinue));
  if (kl_unlikely(!stmtcontinue)) return NULL;
  klast_init(klcast(KlAst*, stmtcontinue), KLAST_STMT_CONTINUE, &klast_stmtcontinue_vfunc);
  return stmtcontinue;
}


static void klast_stmtlet_delete(KlAstStmtLet* stmtlet) {
  free(stmtlet->lvals);
  klast_delete(stmtlet->rvals);
  free(stmtlet);
}

static void klast_stmtassign_delete(KlAstStmtAssign* stmtassign) {
  klast_delete(stmtassign->lvals);
  klast_delete(stmtassign->rvals);
  free(stmtassign);
}

static void klast_stmtexpr_delete(KlAstStmtExpr* stmtexpr) {
  klast_delete(stmtexpr->expr);
  free(stmtexpr);
}

static void klast_stmtif_delete(KlAstStmtIf* stmtif) {
  klast_delete(stmtif->cond);
  klast_delete(stmtif->if_block);
  if (stmtif->else_block)
    klast_delete(stmtif->else_block);
  free(stmtif);
}

static void klast_stmtvfor_delete(KlAstStmtVFor* stmtvfor) {
  free(stmtvfor);
}

static void klast_stmtifor_delete(KlAstStmtIFor* stmtifor) {
  klast_delete(stmtifor->begin);
  klast_delete(stmtifor->end);
  if (stmtifor->step)
    klast_delete(stmtifor->step);
  free(stmtifor);
}

static void klast_stmtgfor_delete(KlAstStmtGFor* stmtgfor) {
  klast_delete(stmtgfor->expr);
  free(stmtgfor->ids);
  free(stmtgfor);
}

static void klast_stmtcfor_delete(KlAstStmtCFor* stmtcfor) {
  if (stmtcfor->init)
    klast_delete(stmtcfor->init);
  if (stmtcfor->cond)
    klast_delete(stmtcfor->cond);
  if (stmtcfor->post)
    klast_delete(stmtcfor->post);
  free(stmtcfor);
}

static void klast_stmtwhile_delete(KlAstStmtWhile* stmtwhile) {
  klast_delete(stmtwhile->cond);
  klast_delete(stmtwhile->block);
  free(stmtwhile);
}

static void klast_stmtblock_delete(KlAstStmtBlock* stmtblock) {
  size_t nstmt = stmtblock->nstmt;
  KlAst** stmts = stmtblock->stmts;
  for (size_t i = 0; i < nstmt; ++i)
    klast_delete(stmts[i]);
  free(stmts);
  free(stmtblock);
}

static void klast_stmtrepeat_delete(KlAstStmtRepeat* stmtrepeat) {
  klast_delete(stmtrepeat->block);
  klast_delete(stmtrepeat->cond);
  free(stmtrepeat);
}

static void klast_stmtreturn_delete(KlAstStmtReturn* stmtreturn) {
  if (stmtreturn->retval)
    klast_delete(stmtreturn->retval);
  free(stmtreturn);
}

static void klast_stmtbreak_delete(KlAstStmtBreak* stmtbreak) {
  free(stmtbreak);
}

static void klast_stmtcontinue_delete(KlAstStmtContinue* stmtcontinue) {
  free(stmtcontinue);
}

