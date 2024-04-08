#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_STMT_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_STMT_H
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klstrtab.h"
#include <stddef.h>


typedef struct tagKlCstStmtLet {
  KlCst base;
  KlStrDesc* lvals;         /* list of identifiers */
  size_t nlval;             /* number of ids */
  KlCst* rvals;             /* right values(must be tuple or single value). */
} KlCstStmtLet;

typedef struct tagKlCstStmtAssign {
  KlCst base;
  KlCst* lvals;             /* left values(single value or tuple) */
  KlCst* rvals;             /* right values(single value or tuple) */
} KlCstStmtAssign;

typedef struct tagKlCstStmtExpr {
  KlCst base;
  KlCst* expr;
} KlCstStmtExpr;

typedef struct tagKlCstStmtIf {
  KlCst base;
  KlCst* cond;
  KlCst* if_block;
  KlCst* else_block;        /* optional. no else block if NULL */
} KlCstStmtIf;

/* variable arguments for */
typedef struct tagKlCstStmtVFor {
  KlCst base;
  KlStrDesc* ids;
  size_t nid;
  KlCst* block;
} KlCstStmtVFor;

/* integer for */
typedef struct tagKlCstStmtIFor {
  KlCst base;
  KlStrDesc id;
  KlCst* begin;
  KlCst* end;
  KlCst* step;    /* nil if NULL */
  KlCst* block;
} KlCstStmtIFor;

/* generic for */
typedef struct tagKlCstStmtGFor {
  KlCst base;
  KlStrDesc* ids;
  size_t nid;
  KlCst* expr;
  KlCst* block;
} KlCstStmtGFor;

/* c-style for */
typedef struct tagKlCstStmtCFor {
  KlCst base;
  KlCst* init;
  KlCst* cond;
  KlCst* post;
  KlCst* block;
} KlCstStmtCFor;

typedef struct tagKlCstStmtWhile {
  KlCst base;
  KlCst* cond;
  KlCst* block;
} KlCstStmtWhile;

typedef struct tagKlCstStmtList {
  KlCst base;
  KlCst** stmts;
  size_t nstmt;
} KlCstStmtList;

typedef struct tagKlCstStmtRepeat {
  KlCst base;
  KlCst* block;
  KlCst* cond;
} KlCstStmtRepeat;

typedef struct tagKlCstStmtReturn {
  KlCst base;
  KlCst* retval;  /* single value or tuple. no return value if NULL */
} KlCstStmtReturn;

typedef struct tagKlCstStmtBreak {
  KlCst base;
} KlCstStmtBreak;

typedef struct tagKlCstStmtContinue {
  KlCst base;
} KlCstStmtContinue;

KlCstStmtLet* klcst_stmtlet_create(KlStrDesc* lvals, size_t nlval, KlCst* rvals, KlFileOffset begin, KlFileOffset end);
KlCstStmtAssign* klcst_stmtassign_create(KlCst* lvals, KlCst* rvals, KlFileOffset begin, KlFileOffset end);
KlCstStmtExpr* klcst_stmtexpr_create(KlCst* expr, KlFileOffset begin, KlFileOffset end);
KlCstStmtIf* klcst_stmtif_create(KlCst* cond, KlCst* if_block, KlCst* else_block, KlFileOffset begin, KlFileOffset end);
KlCstStmtVFor* klcst_stmtvfor_create(KlStrDesc* ids, size_t nid, KlCst* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtIFor* klcst_stmtifor_create(KlStrDesc id, KlCst* ibegin, KlCst* iend, KlCst* istep, KlCst* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtGFor* klcst_stmtgfor_create(KlStrDesc* ids, size_t nid, KlCst* expr, KlCst* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtWhile* klcst_stmtwhile_create(KlCst* cond, KlCst* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtList* klcst_stmtlist_create(KlCst** stmts, size_t nstmt, KlFileOffset begin, KlFileOffset end);
KlCstStmtRepeat* klcst_stmtrepeat_create(KlCst* block, KlCst* cond, KlFileOffset begin, KlFileOffset end);
KlCstStmtReturn* klcst_stmtreturn_create(KlCst* retval, KlFileOffset begin, KlFileOffset end);
KlCstStmtBreak* klcst_stmtbreak_create(KlFileOffset begin, KlFileOffset end);
KlCstStmtContinue* klcst_stmtcontinue_create(KlFileOffset begin, KlFileOffset end);

bool klcst_mustreturn(KlCstStmtList* stmtlist);

#endif
