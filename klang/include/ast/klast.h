#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_H

#define klast_classfullname(suffix) KlAst##suffix
#define klast_create(suffix)   ((klast_classfullname(suffix)*)malloc(sizeof (klast_classfullname(suffix))))


typedef enum tagKlAstNodeType {
  KLAST_EXPR_ARR, KLAST_EXPR_UNIT = KLAST_EXPR_ARR,
  KLAST_EXPR_MAP,
  KLAST_EXPR_CLASS,
  KLAST_EXPR_CONSTANT,
  KLAST_EXPR_ID,
  KLAST_EXPR_TUPLE, KLAST_EXPR_UNIT_END = KLAST_EXPR_TUPLE,

  KLAST_EXPR_NEG, KLAST_EXPR_PRE = KLAST_EXPR_NEG, KLAST_EXPR,
  KLAST_EXPT_NOT,
  KLAST_EXPR_POS, KLAST_EXPR_PRE_END = KLAST_EXPR_POS,

  KLAST_EXPR_INDEX, KLAST_EXPR_POST = KLAST_EXPR_INDEX,
  KLAST_EXPR_DOT,
  KLAST_EXPR_CALL, KLAST_EXPR_POST_END = KLAST_EXPR_CALL,

  KLAST_EXPR_ADD, KLAST_EXPR_BIN = KLAST_EXPR_ADD,
  KLAST_EXPR_SUB,
  KLAST_EXPR_MUL,
  KLAST_EXPR_DIV,
  KLAST_EXPR_MOD,
  KLAST_EXPR_CONCAT,
  KLAST_EXPR_AND,
  KLAST_EXPR_OR,
  KLAST_EXPR_LT, KLAST_EXPR_COMPARE = KLAST_EXPR_LT,
  KLAST_EXPR_GT,
  KLAST_EXPR_LE,
  KLAST_EXPR_GE,
  KLAST_EXPR_EQ,
  KLAST_EXPR_NE, KLAST_EXPR_COMPARE_END = KLAST_EXPR_NE, KLAST_EXPR_BIN_END = KLAST_EXPR_NE,

  KLAST_EXPR_TRI, KLAST_EXPR_END,

  KLAST_STMT_LET, KLAST_STMT = KLAST_STMT_LET,
  KLAST_STMT_ASSIGN,
  KLAST_STMT_EXPR,
  KLAST_STMT_IF,
  KLAST_STMT_VFOR,
  KLAST_STMT_IFOR,
  KLAST_STMT_GFOR,
  KLAST_STMT_CFOR,  /* c-style for */
  KLAST_STMT_WHILE,
  KLAST_STMT_BlOCK,
  KLAST_STMT_REPEAT, KLAST_STMT_END = KLAST_STMT_REPEAT,
} KlAstNodeType;



#endif