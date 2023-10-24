#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H

#include "klang/include/ast/klast_base.h"
#include "klang/include/value/value.h"

typedef enum tagKlAstBinOp {
  KLAST_BINOP_ADD,
  KLAST_BINOP_SUB,
  KLAST_BINOP_MUL,
  KLAST_BINOP_DIV,
  KLAST_BINOP_CON,
} KlAstBinOp;

typedef enum tagKlAstPreOp {
  KLAST_PREOP_POS,
  KLAST_PREOP_NEG,
} KlAstPreOp;

typedef enum tagKlAstPostOp {
  KLAST_POSTOP_INDEX,
  KLAST_POSTOP_CALL,
} KlAstPostOp;

typedef enum tagKlAstUnit {
  KLAST_UNIT_MAP,
  KLAST_UNIT_ARR,
  KLAST_UNIT_STR,
  KLAST_UNIT_NUM,
  KLAST_UNIT_CLO,
} KlAstUnitType;

typedef struct tagKlAstExprBin {
  KlAstBase base;
  KlAstBinOp op;
  KlAstBase* loprand;
  KlAstBase* roprand;
} KlAstExprBin;

typedef struct tagKlAstExprPre {
  KlAstBase base;
  KlAstBinOp op;
  KlAstBase* oprand;
} KlAstExprPre;

typedef struct tagKlAstExprPost {
  KlAstBase base;
  KlAstBinOp op;
  KlAstBase* oprand;
} KlAstExprPost;

typedef struct tagKlAstExprUnit {
  KlAstBase base;
  KlAstUnitType type;
  union {
    KlInt intval;
    KString string;
    KlAstBase* array;
    KlAstBase* map;
    KlAstBase* closure;
  } val;
} KlAstExprUnit;

if(a != 0, () -> {
    print("Hello");
  }, () -> {
    print("Oh...");
  }
);

if = (cond, then_blk, else_blk):
  cond and then_blk or else_blk;


#endif
