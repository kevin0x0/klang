#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_H


#include "include/cst/klstrtbl.h"
#include "include/error/klerror.h"
#include "include/lang/kltypes.h"
#include "include/parse/kltokens.h"
#include <stdlib.h>

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
  KLCST_EXPR_YIELD,

  KLCST_EXPR_POST,
  KLCST_EXPR_CALL,
  KLCST_EXPR_DOT,
  KLCST_EXPR_FUNC,

  KLCST_EXPR_BIN,

  KLCST_EXPR_WHERE, KLCST_EXPR_END = KLCST_EXPR_WHERE,

  KLCST_STMT_LET, KLCST_STMT = KLCST_STMT_LET,
  KLCST_STMT_ASSIGN,
  KLCST_STMT_EXPR,
  KLCST_STMT_IF,
  KLCST_STMT_VFOR,
  KLCST_STMT_IFOR,
  KLCST_STMT_GFOR,
  KLCST_STMT_WHILE,
  KLCST_STMT_BLOCK,
  KLCST_STMT_REPEAT,
  KLCST_STMT_RETURN,
  KLCST_STMT_BREAK,
  KLCST_STMT_CONTINUE, KLCST_STMT_END = KLCST_STMT_CONTINUE,
} KlCstKind;

typedef KlLangInt KlCInt;
typedef KlLangFloat KlCFloat;
typedef KlLangBool KlCBool;

#define KLC_TRUE  (1)
#define KLC_FALSE (0)

typedef enum tagKlCType {
  KLC_INT = 0,
  KLC_FLOAT,
  KLC_NIL,
  KLC_BOOL,
  KLC_STRING,
} KlCType;


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

typedef struct tagKlConstant {
  KlCType type;                      /* boolean, integer, string or nil */
  union {
    KlStrDesc string;               /* literal string */
    KlCInt intval;
    KlCFloat floatval;
    KlCBool boolval;
  };
} KlConstant;


/* forward declaration */
typedef struct tagKlCstTuple KlCstTuple;
typedef struct tagKlCstStmtList KlCstStmtList;

typedef struct tagKlCstClassFieldDesc {
  KlStrDesc name;
  bool shared;
} KlCstClassFieldDesc;

typedef struct tagKlCstIdentifier {
  KlCst base;
  KlStrDesc id;                     /* identifier */
  size_t stkid;                     /* used by code generator for pattern-match and pattern-extract */
} KlCstIdentifier;

typedef struct tagKlCstMap {
  KlCst base;
  KlCst** keys;                     /* keys */
  KlCst** vals;                     /* values */
  size_t npair;                     /* number of k-v pairs */
} KlCstMap;

typedef struct tagKlCstArray {
  KlCst base;
  KlCstTuple* exprlist;             /* tuple */
} KlCstArray;

typedef struct tagKlCstArrayGenerator {
  KlCst base;
  KlStrDesc arrid;                  /* temporary identifier for array */
  KlCstStmtList* block;             /* code that generates an array */
} KlCstArrayGenerator;

typedef struct tagKlCstClass {
  KlCst base;
  KlCstClassFieldDesc* fields;      /* names of fields */
  KlCst** vals;                     /* values */
  size_t nfield;
  KlCst* baseclass;
} KlCstClass;

typedef struct tagKlCstConstant {
  KlCst base;
  KlConstant con;
} KlCstConstant;

typedef struct tagKlCstVararg {
  KlCst base;
} KlCstVararg;

typedef struct tagKlCstTuple {
  KlCst base;
  KlCst** elems;                      /* elements of tuple */
  size_t nelem;                       /* number of elements */
} KlCstTuple;

typedef struct tagKlCstBin {
  KlCst base;
  KlTokenKind op;
  KlCst* loperand;
  KlCst* roperand;
} KlCstBin;

typedef struct tagKlCstPre {
  KlCst base;
  KlTokenKind op;
  KlCst* operand;
} KlCstPre;

typedef struct tagKlCstNew {
  KlCst base;
  KlCst* klclass;
  KlCstTuple* args;   /* parameters for new operator */
} KlCstNew;

typedef struct tagKlCstYield {
  KlCst base;
  KlCstTuple* vals;
} KlCstYield;

typedef struct tagKlCstPost {
  KlCst base;
  KlTokenKind op;
  KlCst* operand;
  KlCst* post;
} KlCstPost;

typedef struct tagKlCstCall {
  KlCst base;
  KlCst* callable;
  KlCstTuple* args;
} KlCstCall;

typedef struct tagKlCstFunc {
  KlCst base;
  KlCstStmtList* block; /* function body */
  KlCstTuple* params;
  bool vararg;          /* has variable argument */
  bool is_method;
} KlCstFunc;

typedef struct tagKlCstDot {
  KlCst base;
  KlCst* operand;
  KlStrDesc field;
} KlCstDot;

typedef struct tagKlCstWhere {
  KlCst base;
  KlCst* expr;
  KlCstStmtList* block;
} KlCstWhere;


typedef struct tagKlCstStmtList {
  KlCst base;
  KlCst** stmts;
  size_t nstmt;
} KlCstStmtList;

/* statements */
typedef struct tagKlCstStmtLet {
  KlCst base;
  KlCstTuple* lvals;        /* left values(tuple) */
  KlCstTuple* rvals;        /* right values(must be tuple or single value). */
} KlCstStmtLet;

typedef struct tagKlCstStmtAssign {
  KlCst base;
  KlCstTuple* lvals;        /* left values(single value or tuple) */
  KlCstTuple* rvals;        /* right values(single value or tuple) */
} KlCstStmtAssign;

typedef struct tagKlCstStmtExpr {
  KlCst base;
  KlCstTuple* exprlist;
} KlCstStmtExpr;

typedef struct tagKlCstStmtIf {
  KlCst base;
  KlCst* cond;
  KlCstStmtList* if_block;
  KlCstStmtList* else_block;  /* optional. no else block if NULL */
} KlCstStmtIf;

/* variable arguments for */
typedef struct tagKlCstStmtVFor {
  KlCst base;
  KlCstTuple* lvals;
  KlCstStmtList* block;
} KlCstStmtVFor;

/* integer for */
typedef struct tagKlCstStmtIFor {
  KlCst base;
  KlCst* lval;
  KlCst* begin;
  KlCst* end;
  KlCst* step;    /* nil if NULL */
  KlCstStmtList* block;
} KlCstStmtIFor;

/* generic for */
typedef struct tagKlCstStmtGFor {
  KlCst base;
  KlCstTuple* lvals;
  KlCst* expr;
  KlCstStmtList* block;
} KlCstStmtGFor;

typedef struct tagKlCstStmtWhile {
  KlCst base;
  KlCst* cond;
  KlCstStmtList* block;
} KlCstStmtWhile;

typedef struct tagKlCstStmtRepeat {
  KlCst base;
  KlCstStmtList* block;
  KlCst* cond;
} KlCstStmtRepeat;

typedef struct tagKlCstStmtReturn {
  KlCst base;
  KlCstTuple* retvals;  /* tuple. */
} KlCstStmtReturn;

typedef struct tagKlCstStmtBreak {
  KlCst base;
} KlCstStmtBreak;

typedef struct tagKlCstStmtContinue {
  KlCst base;
} KlCstStmtContinue;

/* general functions */
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


/* expressions */
KlCstIdentifier* klcst_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end);
KlCstMap* klcst_map_create(KlCst** keys, KlCst** vals, size_t npair, KlFileOffset begin, KlFileOffset end);
KlCstArray* klcst_array_create(KlCstTuple* exprlist, KlFileOffset begin, KlFileOffset end);
KlCstArrayGenerator* klcst_arraygenerator_create(KlStrDesc arrid, KlCstStmtList* stmts, KlFileOffset begin, KlFileOffset end);
KlCstClass* klcst_class_create(KlCstClassFieldDesc* fields, KlCst** vals, size_t nfield, KlCst* base, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_nil(KlFileOffset begin, KlFileOffset end);
KlCstVararg* klcst_vararg_create(KlFileOffset begin, KlFileOffset end);
KlCstTuple* klcst_tuple_create(KlCst** elems, size_t nelem, KlFileOffset begin, KlFileOffset end);
KlCstBin* klcst_bin_create(KlTokenKind op, KlCst* loperand, KlCst* roperand, KlFileOffset begin, KlFileOffset end);
KlCstPre* klcst_pre_create(KlTokenKind op, KlCst* operand, KlFileOffset begin, KlFileOffset end);
KlCstNew* klcst_new_create(KlCst* klclass, KlCstTuple* args, KlFileOffset begin, KlFileOffset end);
KlCstYield* klcst_yield_create(KlCstTuple* vals, KlFileOffset begin, KlFileOffset end);
KlCstPost* klcst_post_create(KlTokenKind op, KlCst* operand, KlCst* post, KlFileOffset begin, KlFileOffset end);
KlCstCall* klcst_call_create(KlCst* callable, KlCstTuple* args, KlFileOffset begin, KlFileOffset end);
KlCstFunc* klcst_func_create(KlCstStmtList* block, KlCstTuple* params, bool vararg, bool is_method, KlFileOffset begin, KlFileOffset end);
KlCstDot* klcst_dot_create(KlCst* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end);
KlCstWhere* klcst_where_create(KlCst* expr, KlCstStmtList* block, KlFileOffset begin, KlFileOffset end);

static inline bool klcst_isboolexpr(KlCst* cst) {
  if (klcst_kind(cst) == KLCST_EXPR_PRE) {
    return klcast(KlCstPre*, cst)->op == KLTK_NOT;
  } else if (klcst_kind(cst) == KLCST_EXPR_BIN) {
    return klcast(KlCstBin*, cst)->op == KLTK_AND ||
           klcast(KlCstBin*, cst)->op == KLTK_OR  ||
           kltoken_isrelation(klcast(KlCstBin*, cst)->op);
  } else {
    return false;
  }
}


static inline void klcst_tuple_shallow_replace(KlCstTuple* tuple, KlCst** elems, size_t nelem) {
  free(tuple->elems);
  tuple->elems = elems;
  tuple->nelem = nelem;
}

/* statements */
KlCstStmtList* klcst_stmtlist_create(KlCst** stmts, size_t nstmt, KlFileOffset begin, KlFileOffset end);
KlCstStmtLet* klcst_stmtlet_create(KlCstTuple* lvals, KlCstTuple* rvals, KlFileOffset begin, KlFileOffset end);
KlCstStmtAssign* klcst_stmtassign_create(KlCstTuple* lvals, KlCstTuple* rvals, KlFileOffset begin, KlFileOffset end);
KlCstStmtExpr* klcst_stmtexpr_create(KlCstTuple* exprlist, KlFileOffset begin, KlFileOffset end);
KlCstStmtIf* klcst_stmtif_create(KlCst* cond, KlCstStmtList* then_block, KlCstStmtList* else_block, KlFileOffset begin, KlFileOffset end);
KlCstStmtVFor* klcst_stmtvfor_create(KlCstTuple* lvals, KlCstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtIFor* klcst_stmtifor_create(KlCst* lval, KlCst* ibegin, KlCst* iend, KlCst* istep, KlCstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtGFor* klcst_stmtgfor_create(KlCstTuple* lvals, KlCst* expr, KlCstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtWhile* klcst_stmtwhile_create(KlCst* cond, KlCstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlCstStmtRepeat* klcst_stmtrepeat_create(KlCstStmtList* block, KlCst* cond, KlFileOffset begin, KlFileOffset end);
KlCstStmtReturn* klcst_stmtreturn_create(KlCstTuple* retvals, KlFileOffset begin, KlFileOffset end);
KlCstStmtBreak* klcst_stmtbreak_create(KlFileOffset begin, KlFileOffset end);
KlCstStmtContinue* klcst_stmtcontinue_create(KlFileOffset begin, KlFileOffset end);

bool klcst_mustreturn(KlCstStmtList* stmtlist);

#endif
