#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_H


#include "klang/include/parse/kllex.h"
typedef enum tagKlCstType {
  KLCST_EXPR_ARR, KLCST_EXPR_UNIT = KLCST_EXPR_ARR, KLCST_EXPR = KLCST_EXPR_ARR,
  KLCST_EXPR_MAP,
  KLCST_EXPR_CLASS,
  KLCST_EXPR_CONSTANT,
  KLCST_EXPR_ID,
  KLCST_EXPR_TUPLE, KLCST_EXPR_UNIT_END = KLCST_EXPR_TUPLE,

  KLCST_EXPR_NEG, KLCST_EXPR_PRE = KLCST_EXPR_NEG,
  KLCST_EXPR_NOT, KLCST_EXPR_PRE_END = KLCST_EXPR_NOT,

  KLCST_EXPR_INDEX, KLCST_EXPR_POST = KLCST_EXPR_INDEX,
  KLCST_EXPR_DOT,
  KLCST_EXPR_FUNC,
  KLCST_EXPR_CALL, KLCST_EXPR_POST_END = KLCST_EXPR_CALL,

  KLCST_EXPR_ADD, KLCST_EXPR_BIN = KLCST_EXPR_ADD,
  KLCST_EXPR_SUB,
  KLCST_EXPR_MUL,
  KLCST_EXPR_DIV,
  KLCST_EXPR_MOD,
  KLCST_EXPR_CONCAT,
  KLCST_EXPR_AND,
  KLCST_EXPR_OR,
  KLCST_EXPR_LT, KLCST_EXPR_COMPARE = KLCST_EXPR_LT,
  KLCST_EXPR_GT,
  KLCST_EXPR_LE,
  KLCST_EXPR_GE,
  KLCST_EXPR_EQ,
  KLCST_EXPR_NE, KLCST_EXPR_COMPARE_END = KLCST_EXPR_NE, KLCST_EXPR_BIN_END = KLCST_EXPR_NE,

  KLCST_EXPR_TRI, KLCST_EXPR_END = KLCST_EXPR_TRI,

  KLCST_STMT_LET, KLCST_STMT = KLCST_STMT_LET,
  KLCST_STMT_ASSIGN,
  KLCST_STMT_EXPR,
  KLCST_STMT_IF,
  KLCST_STMT_VFOR,
  KLCST_STMT_IFOR,
  KLCST_STMT_GFOR,
  KLCST_STMT_CFOR,  /* c-style for */
  KLCST_STMT_WHILE,
  KLCST_STMT_BlOCK,
  KLCST_STMT_REPEAT,
  KLCST_STMT_RETURN,
  KLCST_STMT_BREAK,
  KLCST_STMT_CONTINUE, KLCST_STMT_END = KLCST_STMT_CONTINUE,
} KlCstType;

#define klcst_is_exprunit(type)     (type >= KLCST_EXPR_UNIT && type <= KLCST_EXPR_UNIT_END)
#define klcst_is_exprpre(type)      (type >= KLCST_EXPR_PRE && type <= KLCST_EXPR_PRE_END)
#define klcst_is_exprpost(type)     (type >= KLCST_EXPR_POST && type <= KLCST_EXPR_POST_END)
#define klcst_is_exprbin(type)      (type >= KLCST_EXPR_BIN && type <= KLCST_EXPR_BIN_END)
#define klcst_is_exprcompare(type)  (type >= KLCST_EXPR_COMPARE && type <= KLCST_EXPR_COMPARE_END)
#define klcst_is_exprtri(type)      (type == KLCST_EXPR_TRI)
#define klcst_is_stmt(type)         (type >= KLCST_STMT && type <= KLCST_STMT_END)
#define klcst_is_expr(type)         (type >= KLCST_EXPR && type <= KLCST_EXPR_END)


typedef struct tagKlCstVirtualFunc KlCstVirtualFunc;

/* this serves as the base class of concrete syntax tree node,
 * and will be contained to the header of any other node.
 */

typedef struct tagKlCst {
  KlCstType type;
  KlCstVirtualFunc* vfunc;
  KlFilePos begin;
  KlFilePos end;
} KlCst;

typedef void (*KlCstDelete)(KlCst* ast);

struct tagKlCstVirtualFunc {
  KlCstDelete astdelete;
};


static inline void klcst_init(KlCst* cst, KlCstType type, KlCstVirtualFunc* vfunc);
static inline void klcst_delete(KlCst* cst);
static inline KlCstType klcst_kind(KlCst* cst);
static inline void klcst_setposition(KlCst* cst, KlFilePos begin, KlFilePos end);

static inline void klcst_init(KlCst* cst, KlCstType type, KlCstVirtualFunc* vfunc) {
  cst->type = type;
  cst->vfunc = vfunc;
}

static inline void klcst_delete(KlCst* cst) {
  cst->vfunc->astdelete(cst);
}

static inline KlCstType klcst_kind(KlCst* cst) {
  return cst->type;
}

static inline void klcst_setposition(KlCst* cst, KlFilePos begin, KlFilePos end) {
  cst->begin = begin;
  cst->end = end;
}

#endif
