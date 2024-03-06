#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_STMT_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_STMT_H
#include "klang/include/cst/klcst.h"
#include "klang/include/parse/klstrtab.h"
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
  KlStrDesc id;
} KlCstStmtVFor;

/* integer for */
typedef struct tagKlCstStmtIFor {
  KlCst base;
  KlStrDesc id;
  KlCst* begin;
  KlCst* end;
  KlCst* step;    /* nil if NULL */
} KlCstStmtIFor;

/* generic for */
typedef struct tagKlCstStmtGFor {
  KlCst base;
  KlStrDesc* ids;
  size_t nid;
  KlCst* expr;
} KlCstStmtGFor;

/* c-style for */
typedef struct tagKlCstStmtCFor {
  KlCst base;
  KlCst* init;
  KlCst* cond;
  KlCst* post;
} KlCstStmtCFor;

typedef struct tagKlCstStmtWhile {
  KlCst base;
  KlCst* cond;
  KlCst* block;
} KlCstStmtWhile;

typedef struct tagKlCstStmtBlock {
  KlCst** stmts;
  size_t nstmt;
} KlCstStmtBlock;

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

KlCstStmtLet* klcst_stmtlet_create(void);
KlCstStmtAssign* klcst_stmtassign_create(void);
KlCstStmtExpr* klcst_stmtexpr_create(void);
KlCstStmtIf* klcst_stmtif_create(void);
KlCstStmtVFor* klcst_stmtvfor_create(void);
KlCstStmtIFor* klcst_stmtifor_create(void);
KlCstStmtGFor* klcst_stmtgfor_create(void);
KlCstStmtCFor* klcst_stmtcfor_create(void);
KlCstStmtWhile* klcst_stmtwhile_create(void);
KlCstStmtBlock* klcst_stmtblock_create(void);
KlCstStmtRepeat* klcst_stmtrepeat_create(void);
KlCstStmtReturn* klcst_stmtreturn_create(void);
KlCstStmtBreak* klcst_stmtbreak_create(void);
KlCstStmtContinue* klcst_stmtcontinue_create(void);

#endif
