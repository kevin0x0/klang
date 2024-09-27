#ifndef _KLANG_INCLUDE_AST_KLAST_H_
#define _KLANG_INCLUDE_AST_KLAST_H_


#include "include/ast/klstrtbl.h"
#include "include/error/klerror.h"
#include "include/lang/kltypes.h"
#include "include/parse/kltokens.h"
#include <stdlib.h>

#define klast_alloc(Type)                   ((Type*)malloc(sizeof (Type)))

#define klast_begin(ast)                    (klcast(KlAst*, (ast))->begin)
#define klast_end(ast)                      (klcast(KlAst*, (ast))->end)
#define klast_setposition(ast, begin, end)  (klast_setposition_raw(klcast(KlAst*, (ast)), (begin), (end)))
#define klast_init(ast, vfunc)              (klast_init_raw(klcast(KlAst*, (ast)), (vfunc)))
#define klast_kind(ast)                     (klast_kind_raw(klcast(KlAst*, (ast))))
#define klast_destroy(ast)                  (klast_destroy_raw(klcast(KlAst*, (ast))))
#define klast(ast)                          (klcast(KlAst*, (ast)))
#define klast_delete(ast)                   do { klast_destroy((ast)); free((ast)); } while (0)


typedef enum tagKlAstKind {
  KLAST_EXPR_LIST,

  KLAST_EXPR_ARRAY, KLAST_EXPR_UNIT = KLAST_EXPR_ARRAY, KLAST_EXPR = KLAST_EXPR_ARRAY,
  KLAST_EXPR_ARRGEN,
  KLAST_EXPR_MAP,
  KLAST_EXPR_MAPGEN,
  KLAST_EXPR_CLASS,
  KLAST_EXPR_CONSTANT,
  KLAST_EXPR_ID,
  KLAST_EXPR_VARARG,
  KLAST_EXPR_TUPLE,

  KLAST_EXPR_PRE,
  KLAST_EXPR_NEW,
  KLAST_EXPR_YIELD,

  KLAST_EXPR_INDEX,
  KLAST_EXPR_APPEND,
  KLAST_EXPR_CALL,
  KLAST_EXPR_DOT,
  KLAST_EXPR_FUNC,

  KLAST_EXPR_BIN,
  KLAST_EXPR_WALRUS,

  KLAST_EXPR_ASYNC,
  KLAST_EXPR_MATCH,
  KLAST_EXPR_WHERE, KLAST_EXPR_END = KLAST_EXPR_WHERE,

  KLAST_STMT_LET, KLAST_STMT = KLAST_STMT_LET,
  KLAST_STMT_METHOD,
  KLAST_STMT_MATCH,
  KLAST_STMT_LOCALFUNC,
  KLAST_STMT_ASSIGN,
  KLAST_STMT_EXPR,
  KLAST_STMT_IF,
  KLAST_STMT_VFOR,
  KLAST_STMT_IFOR,
  KLAST_STMT_GFOR,
  KLAST_STMT_WHILE,
  KLAST_STMT_BLOCK,
  KLAST_STMT_REPEAT,
  KLAST_STMT_RETURN,
  KLAST_STMT_BREAK,
  KLAST_STMT_CONTINUE, KLAST_STMT_END = KLAST_STMT_CONTINUE,
} KlAstKind;

typedef KlLangInt KlCInt;
typedef KlLangUInt KlCUInt;
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

typedef struct tagKlAstInfo KlAstInfo;


typedef struct tagKlAst {
  const KlAstInfo* info;
  KlFileOffset begin;
  KlFileOffset end;
} KlAst;

typedef void (*KlAstDelete)(KlAst* ast);

struct tagKlAstInfo {
  KlAstDelete destructor;
  KlAstKind kind;
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

/* all the expression node should inherit this */
typedef struct tagKlAstExpr {
  KL_DERIVE_FROM(KlAst, _astbase_);
} KlAstExpr;

/* all the statement node should inherit this */
typedef struct tagKlAstStmt {
  KL_DERIVE_FROM(KlAst, _astbase_);
} KlAstStmt;

/* forward declaration */
typedef struct tagKlAstExprList KlAstExprList;
typedef struct tagKlAstStmtList KlAstStmtList;

typedef struct tagKlAstClassFieldDesc {
  KlStrDesc name;
  bool shared;
  bool ismethod;
} KlAstClassFieldDesc;

typedef struct tagKlAstIdentifier {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlStrDesc id;                     /* identifier */
} KlAstIdentifier;

typedef struct tagKlAstMap {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr** keys;                 /* keys */
  KlAstExpr** vals;                 /* values */
  size_t npair;                     /* number of k-v pairs */
} KlAstMap;

typedef struct tagKlAstMapComprehension {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlStrDesc mapid;                  /* temporary identifier for array */
  KlAstStmtList* block;             /* code that generates an array */
} KlAstMapComprehension;

typedef struct tagKlAstArray {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExprList* exprlist;          /* exprlist */
} KlAstArray;

typedef struct tagKlAstTuple {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr** vals;                 /* values of exprlist */
  size_t nval;                      /* number of values */
} KlAstTuple;

typedef struct tagKlAstArrayComprehension {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlStrDesc arrid;                  /* temporary identifier for array */
  KlAstStmtList* block;             /* code that generates an array */
} KlAstArrayComprehension;

typedef struct tagKlAstClass {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstClassFieldDesc* fields;      /* names of fields */
  KlAstExpr** vals;                 /* values */
  size_t nfield;
  KlAstExpr* baseclass;
} KlAstClass;

typedef struct tagKlAstConstant {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlConstant con;
} KlAstConstant;

typedef struct tagKlAstVararg {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
} KlAstVararg;

typedef struct tagKlAstExprList {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr** exprs;                  /* elements of exprlist */
  size_t nexpr;                       /* number of elements */
} KlAstExprList;

typedef struct tagKlAstBin {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlTokenKind op;
  KlAstExpr* loperand;
  KlAstExpr* roperand;
} KlAstBin;

typedef struct tagKlAstWalrus {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* pattern;
  KlAstExpr* rval;
} KlAstWalrus;

typedef struct tagKlAstAsync {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* callable;
} KlAstAsync;

typedef struct tagKlAstPre {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlTokenKind op;
  KlAstExpr* operand;
} KlAstPre;

typedef struct tagKlAstNew {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* klclass;
  KlAstExprList* args;   /* parameters for new operator */
} KlAstNew;

typedef struct tagKlAstYield {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExprList* vals;
} KlAstYield;

typedef struct tagKlAstIndex {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* indexable;
  KlAstExpr* index;
} KlAstIndex;

typedef struct tagKlAstAppend {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* array;
  KlAstExprList* exprlist;
} KlAstAppend;

typedef struct tagKlAstCall {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* callable;
  KlAstExprList* args;
} KlAstCall;

typedef struct tagKlAstFunc {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstStmtList* block; /* function body */
  KlAstExprList* params;
  bool vararg;          /* has variable argument */
} KlAstFunc;

typedef struct tagKlAstDot {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* operand;
  KlStrDesc field;
} KlAstDot;

typedef struct tagKlAstMatch {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* matchobj;
  KlAstExpr** patterns;
  KlAstExpr** exprs;
  size_t npattern;
} KlAstMatch;

typedef struct tagKlAstWhere {
  KL_DERIVE_FROM(KlAstExpr, _astbase_);
  KlAstExpr* expr;
  KlAstStmtList* block;
} KlAstWhere;


typedef struct tagKlAstStmtList {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstStmt** stmts;
  size_t nstmt;
} KlAstStmtList;

/* statements */
typedef struct tagKlAstStmtLet {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* lvals;        /* left values(exprlist) */
  KlAstExprList* rvals;        /* right values(must be exprlist or single value). */
} KlAstStmtLet;

typedef struct tagKlAstStmtMethod {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstDot* lval;
  KlAstExpr* rval;
} KlAstStmtMethod;

typedef struct tagKlAstStmtMatch {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExpr* matchobj;
  KlAstExpr** patterns;
  KlAstStmtList** stmtlists;
  size_t npattern;
} KlAstStmtMatch;

typedef struct tagKlAstStmtLocalDefinition {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlFileOffset idbegin;
  KlFileOffset idend;
  KlStrDesc id;
  KlAstExpr* expr;
} KlAstStmtLocalDefinition;

typedef struct tagKlAstStmtAssign {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* lvals;        /* left values(single value or exprlist) */
  KlAstExprList* rvals;        /* right values(single value or exprlist) */
} KlAstStmtAssign;

typedef struct tagKlAstStmtExpr {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* exprlist;
} KlAstStmtExpr;

typedef struct tagKlAstStmtIf {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExpr* cond;
  KlAstStmtList* then_block;
  KlAstStmtList* else_block;  /* optional. no else block if NULL */
} KlAstStmtIf;

/* variable arguments for */
typedef struct tagKlAstStmtVFor {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* lvals;
  KlAstStmtList* block;
} KlAstStmtVFor;

/* integer for */
typedef struct tagKlAstStmtIFor {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* lval;
  KlAstExpr* begin;
  KlAstExpr* end;
  KlAstExpr* step;            /* nil if NULL */
  KlAstStmtList* block;
} KlAstStmtIFor;

/* generic for */
typedef struct tagKlAstStmtGFor {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* lvals;
  KlAstExpr* expr;
  KlAstStmtList* block;
} KlAstStmtGFor;

typedef struct tagKlAstStmtWhile {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExpr* cond;
  KlAstStmtList* block;
} KlAstStmtWhile;

typedef struct tagKlAstStmtRepeat {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstStmtList* block;
  KlAstExpr* cond;
} KlAstStmtRepeat;

typedef struct tagKlAstStmtReturn {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
  KlAstExprList* retvals;  /* exprlist. */
} KlAstStmtReturn;

typedef struct tagKlAstStmtBreak {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
} KlAstStmtBreak;

typedef struct tagKlAstStmtContinue {
  KL_DERIVE_FROM(KlAstStmt, _astbase_);
} KlAstStmtContinue;

/* general functions */
static inline void klast_init_raw(KlAst* ast, const KlAstInfo* vfunc);
static inline void klast_destroy_raw(KlAst* ast);
static inline KlAstKind klast_kind_raw(KlAst* ast);
static inline void klast_setposition_raw(KlAst* ast, KlFileOffset begin, KlFileOffset end);


static inline void klast_init_raw(KlAst* ast, const KlAstInfo* vfunc) {
  ast->info = vfunc;
}

static inline void klast_destroy_raw(KlAst* ast) {
  ast->info->destructor(ast);
}

static inline KlAstKind klast_kind_raw(KlAst* ast) {
  return ast->info->kind;
}

static inline void klast_setposition_raw(KlAst* ast, KlFileOffset begin, KlFileOffset end) {
  ast->begin = begin;
  ast->end = end;
}


/* expressions */
KlAstIdentifier* klast_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end);
KlAstMap* klast_map_create(KlAstExpr** keys, KlAstExpr** vals, size_t npair, KlFileOffset begin, KlFileOffset end);
KlAstTuple* klast_tuple_create(KlAstExpr** vals, size_t nval, KlFileOffset begin, KlFileOffset end);
KlAstMapComprehension* klast_mapcomprehension_create(KlStrDesc arrid, KlAstStmtList* stmts, KlFileOffset begin, KlFileOffset end);
KlAstArray* klast_array_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end);
KlAstArrayComprehension* klast_arraycomprehension_create(KlStrDesc arrid, KlAstStmtList* stmts, KlFileOffset begin, KlFileOffset end);
KlAstClass* klast_class_create(KlAstClassFieldDesc* fields, KlAstExpr** vals, size_t nfield, KlAstExpr* base, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_nil(KlFileOffset begin, KlFileOffset end);
KlAstVararg* klast_vararg_create(KlFileOffset begin, KlFileOffset end);
KlAstExprList* klast_exprlist_create(KlAstExpr** exprs, size_t nexpr, KlFileOffset begin, KlFileOffset end);
KlAstBin* klast_bin_create(KlTokenKind op, KlAstExpr* loperand, KlAstExpr* roperand, KlFileOffset begin, KlFileOffset end);
KlAstWalrus* klast_walrus_create(KlAstExpr* pattern, KlAstExpr* rval, KlFileOffset begin, KlFileOffset end);
KlAstAsync* klast_async_create(KlAstExpr* callable, KlFileOffset begin, KlFileOffset end);
KlAstPre* klast_pre_create(KlTokenKind op, KlAstExpr* operand, KlFileOffset begin, KlFileOffset end);
KlAstNew* klast_new_create(KlAstExpr* klclass, KlAstExprList* args, KlFileOffset begin, KlFileOffset end);
KlAstYield* klast_yield_create(KlAstExprList* vals, KlFileOffset begin, KlFileOffset end);
KlAstIndex* klast_index_create(KlAstExpr* operand, KlAstExpr* index, KlFileOffset begin, KlFileOffset end);
KlAstAppend* klast_append_create(KlAstExpr* array, KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end);
KlAstCall* klast_call_create(KlAstExpr* callable, KlAstExprList* args, KlFileOffset begin, KlFileOffset end);
KlAstFunc* klast_func_create(KlAstStmtList* block, KlAstExprList* params, bool vararg, KlFileOffset begin, KlFileOffset end);
KlAstDot* klast_dot_create(KlAstExpr* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end);
KlAstMatch* klast_match_create(KlAstExpr* matchobj, KlAstExpr** patterns, KlAstExpr** exprs, size_t npattern, KlFileOffset begin, KlFileOffset end);
KlAstWhere* klast_where_create(KlAstExpr* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);

KlAstExpr* klast_exprlist_stealfirst_and_destroy(KlAstExprList* exprlist);
bool klast_isboolexpr(KlAstExpr* ast);

static inline bool klast_expr_islvalue(KlAstExpr* ast) {
  return klast_kind(ast) == KLAST_EXPR_ID   ||
         klast_kind(ast) == KLAST_EXPR_DOT  ||
         klast_kind(ast) == KLAST_EXPR_INDEX;
}

static inline void klast_exprlist_shallow_replace(KlAstExprList* exprlist, KlAstExpr** exprs, size_t nexpr) {
  free(exprlist->exprs);
  exprlist->exprs = exprs;
  exprlist->nexpr = nexpr;
}

/* statements */
KlAstStmtList* klast_stmtlist_create(KlAstStmt** stmts, size_t nstmt, KlFileOffset begin, KlFileOffset end);
KlAstStmtLet* klast_stmtlet_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end);
KlAstStmtMethod* klast_stmtmethod_create(KlAstDot* lval, KlAstExpr* rval, KlFileOffset begin, KlFileOffset end);
KlAstStmtMatch* klast_stmtmatch_create(KlAstExpr* matchobj, KlAstExpr** patterns, KlAstStmtList** stmtlists, size_t npattern, KlFileOffset begin, KlFileOffset end);
KlAstStmtLocalDefinition* klast_stmtlocaldef_create(KlStrDesc id, KlFileOffset idbegin, KlFileOffset idend, KlAstExpr* expr, KlFileOffset begin, KlFileOffset end);
KlAstStmtAssign* klast_stmtassign_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end);
KlAstStmtExpr* klast_stmtexpr_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end);
KlAstStmtIf* klast_stmtif_create(KlAstExpr* cond, KlAstStmtList* then_block, KlAstStmtList* else_block, KlFileOffset begin, KlFileOffset end);
KlAstStmtVFor* klast_stmtvfor_create(KlAstExprList* lvals, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtIFor* klast_stmtifor_create(KlAstExprList* lval, KlAstExpr* ibegin, KlAstExpr* iend, KlAstExpr* istep, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtGFor* klast_stmtgfor_create(KlAstExprList* lvals, KlAstExpr* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtWhile* klast_stmtwhile_create(KlAstExpr* cond, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtRepeat* klast_stmtrepeat_create(KlAstStmtList* block, KlAstExpr* cond, KlFileOffset begin, KlFileOffset end);
KlAstStmtReturn* klast_stmtreturn_create(KlAstExprList* retvals, KlFileOffset begin, KlFileOffset end);
KlAstStmtBreak* klast_stmtbreak_create(KlFileOffset begin, KlFileOffset end);
KlAstStmtContinue* klast_stmtcontinue_create(KlFileOffset begin, KlFileOffset end);

bool klast_mustreturn(KlAstStmtList* stmtlist);
static inline KlAstExprList* klast_stmtexpr_steal_exprlist_and_destroy(KlAstStmtExpr* stmtexpr);

static inline KlAstExprList* klast_stmtexpr_steal_exprlist_and_destroy(KlAstStmtExpr* stmtexpr) {
  KlAstExprList* exprlist = stmtexpr->exprlist;
  free(stmtexpr);
  return exprlist;
}

#endif
