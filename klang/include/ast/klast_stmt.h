#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_STMT_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_STMT_H
#include "klang/include/ast/klast.h"
#include "klang/include/parse/klstrtab.h"
#include <stddef.h>


typedef struct tagKlAstStmtLet {
  KlAst base;
  KlStrDesc* lvals;         /* list of identifiers */
  size_t nlval;             /* number of ids */
  KlAst* rvals;             /* right values(must be tuple or single value). */
} KlAstStmtLet;

typedef struct tagKlAstStmtAssign {
  KlAst base;
  KlAst* lvals;             /* left values(single value or tuple) */
  KlAst* rvals;             /* right values(single value or tuple) */
} KlAstStmtAssign;

typedef struct tagKlAstStmtExpr {
  KlAst base;
  KlAst* expr;
} KlAstStmtExpr;

typedef struct tagKlAstStmtIf {
  KlAst base;
  KlAst* cond;
  KlAst* if_block;
  KlAst* else_block;        /* optional. no else block if NULL */
} KlAstStmtIf;

/* variable arguments for */
typedef struct tagKlAstStmtVFor {
  KlAst base;
  KlStrDesc id;
} KlAstStmtVFor;

/* integer for */
typedef struct tagKlAstStmtIFor {
  KlAst base;
  KlStrDesc id;
  KlAst* begin;
  KlAst* end;
  KlAst* step;    /* nil if NULL */
} KlAstStmtIFor;

/* generic for */
typedef struct tagKlAstStmtGFor {
  KlAst base;
  KlStrDesc* ids;
  size_t nid;
  KlAst* expr;
} KlAstStmtGFor;

/* c-style for */
typedef struct tagKlAstStmtCFor {
  KlAst base;
  KlAst* init;
  KlAst* cond;
  KlAst* post;
} KlAstStmtCFor;

typedef struct tagKlAstStmtWhile {
  KlAst base;
  KlAst* cond;
  KlAst* block;
} KlAstStmtWhile;

typedef struct tagKlAstStmtBlock {
  KlAst** stmts;
  size_t nstmt;
} KlAstStmtBlock;

typedef struct tagKlAstStmtRepeat {
  KlAst base;
  KlAst* block;
  KlAst* cond;
} KlAstStmtRepeat;

typedef struct tagKlAstStmtReturn {
  KlAst base;
  KlAst* retval;  /* single value or tuple. no return value if NULL */
} KlAstStmtReturn;

typedef struct tagKlAstStmtBreak {
  KlAst base;
} KlAstStmtBreak;

typedef struct tagKlAstStmtContinue {
  KlAst base;
} KlAstStmtContinue;

KlAstStmtLet* klast_stmtlet_create(void);
KlAstStmtAssign* klast_stmtassign_create(void);
KlAstStmtExpr* klast_stmtexpr_create(void);
KlAstStmtIf* klast_stmtif_create(void);
KlAstStmtVFor* klast_stmtvfor_create(void);
KlAstStmtIFor* klast_stmtifor_create(void);
KlAstStmtGFor* klast_stmtgfor_create(void);
KlAstStmtCFor* klast_stmtcfor_create(void);
KlAstStmtWhile* klast_stmtwhile_create(void);
KlAstStmtBlock* klast_stmtblock_create(void);
KlAstStmtRepeat* klast_stmtrepeat_create(void);
KlAstStmtReturn* klast_stmtreturn_create(void);
KlAstStmtBreak* klast_stmtbreak_create(void);
KlAstStmtContinue* klast_stmtcontinue_create(void);

#endif
