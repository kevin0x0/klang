#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_STMT_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_STMT_H
#include "klang/include/ast/klast_base.h"
#include "klang/include/parse/klstrtab.h"
#include <stddef.h>


typedef struct tagKlAstStmtLet {
  KlAstBase base;
  KlStrDesc* lvals;         /* list of identifiers */
  size_t nlval;             /* number of ids */
  KlAstBase* rvals;         /* right values(must be tuple or single value). */
} KlAstStmtLet;

typedef struct tagKlAstStmtAssign {
  KlAstBase base;
  KlAstBase* lvals;         /* left values(single value or tuple) */
  KlAstBase* rvals;         /* right values(single value or tuple) */
} KlAstStmtAssign;

typedef struct tagKlAstStmtExpr {
  KlAstBase base;
  KlAstBase* expr;
} KlAstStmtExpr;

typedef struct tagKlAstStmtIf {
  KlAstBase base;
  KlAstBase* cond;
  KlAstBase* if_block;
  KlAstBase* else_block;    /* optional. no else block if NULL */
} KlAstStmtIf;

/* variable arguments for */
typedef struct tagKlAstStmtVFor {
  KlAstBase base;
  KlStrDesc id;
} KlAstStmteVFor;

/* integer for */
typedef struct tagKlAstStmtIFor {
  KlAstBase base;
  KlStrDesc id;
  KlAstBase* begin;
  KlAstBase* end;
  KlAstBase* step;
} KlAstStmteIFor;

/* generic for */
typedef struct tagKlAstStmtGFor {
  KlAstBase base;
  KlStrDesc* ids;
  size_t nid;
  KlAstBase* expr;
} KlAstStmteGFor;

/* c-style for */
typedef struct tagKlAstStmtCFor {
  KlAstBase base;
  KlStrDesc* ids;
  size_t nid;
  KlAstBase* expr;
} KlAstStmteCFor;

typedef struct tagKlAstStmtWhile {
  KlAstBase base;
  KlAstBase* cond;
  KlAstBase* block;
} KlAstStmtWhile;

typedef struct tagKlAstStmtBlock {
  KlAstBase** stmts;
  size_t nstmt;
} KlAstStmtBlock;

typedef struct tagKlAstStmtRepeat {
  KlAstBase base;
  KlAstBase* block;
  KlAstBase* cond;
} KlAstStmtRepeat;

#endif
