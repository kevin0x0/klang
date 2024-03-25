#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_H


#include "klang/include/parse/kllex.h"

#define klcst_alloc(Type)                   ((Type*)malloc(sizeof (Type)))

#define klcst_begin(cst)                    (klcast(KlCst*, (cst))->begin)
#define klcst_end(cst)                      (klcast(KlCst*, (cst))->end)
#define klcst_setposition(cst, begin, end)  (klcst_setposition_raw(klcast(KlCst*, (cst)), (begin), (end)))
#define klcst_init(cst, vfunc)              (klcst_init_raw(klcast(KlCst*, (cst)), (vfunc)))
#define klcst_kind(cst)                     (klcst_kind_raw(klcast(KlCst*, (cst))))
#define klcst_destroy(cst)                  (klcst_destroy_raw(klcast(KlCst*, (cst))))
#define klcst_delete(cst)                   (klcst_delete_raw(klcast(KlCst*, (cst))))
#define klcst(cst)                          (klcast(KlCst*, (cst)))


typedef enum tagKlCstKind {
  KLCST_EXPR_ARR, KLCST_EXPR_UNIT = KLCST_EXPR_ARR, KLCST_EXPR = KLCST_EXPR_ARR,
  KLCST_EXPR_ARRGEN,
  KLCST_EXPR_MAP,
  KLCST_EXPR_CLASS,
  KLCST_EXPR_CONSTANT,
  KLCST_EXPR_ID,
  KLCST_EXPR_VARARG,
  KLCST_EXPR_TUPLE, KLCST_EXPR_UNIT_END = KLCST_EXPR_TUPLE,

  KLCST_EXPR_PRE,
  KLCST_EXPR_NEW,

  KLCST_EXPR_INDEX, KLCST_EXPR_POST = KLCST_EXPR_INDEX,
  KLCST_EXPR_DOT,
  KLCST_EXPR_FUNC,
  KLCST_EXPR_ARRPUSH,
  KLCST_EXPR_CALL, KLCST_EXPR_POST_END = KLCST_EXPR_CALL,

  KLCST_EXPR_BIN,

  KLCST_EXPR_SEL, KLCST_EXPR_END = KLCST_EXPR_SEL,

  KLCST_STMT_LET, KLCST_STMT = KLCST_STMT_LET,
  KLCST_STMT_ASSIGN,
  KLCST_STMT_EXPR,
  KLCST_STMT_IF,
  KLCST_STMT_VFOR,
  KLCST_STMT_IFOR,
  KLCST_STMT_GFOR,
  KLCST_STMT_CFOR,  /* c-style for */
  KLCST_STMT_WHILE,
  KLCST_STMT_BLOCK,
  KLCST_STMT_REPEAT,
  KLCST_STMT_RETURN,
  KLCST_STMT_BREAK,
  KLCST_STMT_CONTINUE, KLCST_STMT_END = KLCST_STMT_CONTINUE,
} KlCstKind;

#define klcst_is_exprunit(type)     (type >= KLCST_EXPR_UNIT && type <= KLCST_EXPR_UNIT_END)
#define klcst_is_exprpre(type)      (type >= KLCST_EXPR_PRE && type <= KLCST_EXPR_PRE_END)
#define klcst_is_exprpost(type)     (type >= KLCST_EXPR_POST && type <= KLCST_EXPR_POST_END)
#define klcst_is_exprbin(type)      (type == KLCST_EXPR_BIN)
#define klcst_is_exprcompare(type)  (type >= KLCST_EXPR_COMPARE && type <= KLCST_EXPR_COMPARE_END)
#define klcst_is_exprter(type)      (type == KLCST_EXPR_TER)
#define klcst_is_stmt(type)         (type >= KLCST_STMT && type <= KLCST_STMT_END)
#define klcst_is_expr(type)         (type >= KLCST_EXPR && type <= KLCST_EXPR_END)


typedef struct tagKlCstInfo KlCstInfo;

/* this serves as the base class of concrete syntax tree node,
 * and will be contained to the header of any other node.
 */

typedef struct tagKlCst {
  KlCstInfo* info;
  KlFileOffset begin;
  KlFileOffset end;
} KlCst;

typedef void (*KlCstDelete)(KlCst* ast);

struct tagKlCstInfo {
  KlCstDelete destructor;
  KlCstKind kind;
};

static inline void klcst_init_raw(KlCst* cst, KlCstInfo* vfunc);
static inline void klcst_delete_raw(KlCst* cst);
static inline void klcst_destroy_raw(KlCst* cst);
static inline KlCstKind klcst_kind_raw(KlCst* cst);
static inline void klcst_setposition_raw(KlCst* cst, KlFileOffset begin, KlFileOffset end);

static inline void klcst_init_raw(KlCst* cst, KlCstInfo* vfunc) {
  cst->info = vfunc;
}

static inline void klcst_delete_raw(KlCst* cst) {
  cst->info->destructor(cst);
  free(cst);
}

static inline void klcst_destroy_raw(KlCst* cst) {
  cst->info->destructor(cst);
}

static inline KlCstKind klcst_kind_raw(KlCst* cst) {
  return cst->info->kind;
}

static inline void klcst_setposition_raw(KlCst* cst, KlFileOffset begin, KlFileOffset end) {
  cst->begin = begin;
  cst->end = end;
}

#endif
