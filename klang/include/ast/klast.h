#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_H


typedef enum tagKlAstType {
  KLAST_EXPR_ARR, KLAST_EXPR_UNIT = KLAST_EXPR_ARR, KLAST_EXPR = KLAST_EXPR_ARR,
  KLAST_EXPR_MAP,
  KLAST_EXPR_CLASS,
  KLAST_EXPR_CONSTANT,
  KLAST_EXPR_ID,
  KLAST_EXPR_TUPLE, KLAST_EXPR_UNIT_END = KLAST_EXPR_TUPLE,

  KLAST_EXPR_NEG, KLAST_EXPR_PRE = KLAST_EXPR_NEG,
  KLAST_EXPT_NOT,
  KLAST_EXPR_POS, KLAST_EXPR_PRE_END = KLAST_EXPR_POS,

  KLAST_EXPR_INDEX, KLAST_EXPR_POST = KLAST_EXPR_INDEX,
  KLAST_EXPR_DOT,
  KLAST_EXPR_FUNC,
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

  KLAST_EXPR_TRI, KLAST_EXPR_END = KLAST_EXPR_TRI,

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
  KLAST_STMT_REPEAT,
  KLAST_STMT_RETURN,
  KLAST_STMT_BREAK,
  KLAST_STMT_CONTINUE, KLAST_STMT_END = KLAST_STMT_CONTINUE,
} KlAstType;

#define klast_is_exprunit(type)     (type >= KLAST_EXPR_UNIT && type <= KLAST_EXPR_UNIT_END)
#define klast_is_exprpre(type)      (type >= KLAST_EXPR_PRE && type <= KLAST_EXPR_PRE_END)
#define klast_is_exprpost(type)     (type >= KLAST_EXPR_POST && type <= KLAST_EXPR_POST_END)
#define klast_is_exprbin(type)      (type >= KLAST_EXPR_BIN && type <= KLAST_EXPR_BIN_END)
#define klast_is_exprcompare(type)  (type >= KLAST_EXPR_COMPARE && type <= KLAST_EXPR_COMPARE_END)
#define klast_is_exprtri(type)      (type == KLAST_EXPR_TRI)
#define klast_is_stmt(type)         (type >= KLAST_STMT && type <= KLAST_STMT_END)
#define klast_is_expr(type)         (type >= KLAST_EXPR && type <= KLAST_EXPR_END)


typedef struct tagKlAstVirtualFunc KlAstVirtualFunc;

/* this serves as the base class of abstract syntax tree node,
 * and will be contained to the header of any other node.
 */

typedef struct tagKlAst {
  KlAstType type;
  KlAstVirtualFunc* vfunc;
} KlAst;

typedef void (*KlAstDelete)(KlAst* ast);

struct tagKlAstVirtualFunc {
  KlAstDelete astdelete;
};


static inline void klast_init(KlAst* ast, KlAstType type, KlAstVirtualFunc* vfunc);
static inline void klast_delete(KlAst* ast);
static inline KlAstType klast_type(KlAst* ast);

static inline void klast_init(KlAst* ast, KlAstType type, KlAstVirtualFunc* vfunc) {
  ast->type = type;
  ast->vfunc = vfunc;
}

static inline void klast_delete(KlAst* ast) {
  ast->vfunc->astdelete(ast);
}

static inline KlAstType klast_type(KlAst* ast) {
  return ast->type;
}


#endif
