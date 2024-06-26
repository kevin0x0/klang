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
  KLAST_EXPR_ARR, KLAST_EXPR_UNIT = KLAST_EXPR_ARR, KLAST_EXPR = KLAST_EXPR_ARR,
  KLAST_EXPR_ARRGEN,
  KLAST_EXPR_MAP,
  KLAST_EXPR_MAPGEN,
  KLAST_EXPR_CLASS,
  KLAST_EXPR_CONSTANT,
  KLAST_EXPR_ID,
  KLAST_EXPR_VARARG,
  KLAST_EXPR_LIST, KLAST_EXPR_UNIT_END = KLAST_EXPR_LIST,

  KLAST_EXPR_PRE,
  KLAST_EXPR_NEW,
  KLAST_EXPR_YIELD,

  KLAST_EXPR_POST,
  KLAST_EXPR_CALL,
  KLAST_EXPR_DOT,
  KLAST_EXPR_FUNC,

  KLAST_EXPR_BIN,

  KLAST_EXPR_MATCH,
  KLAST_EXPR_WHERE, KLAST_EXPR_END = KLAST_EXPR_WHERE,

  KLAST_STMT_LET, KLAST_STMT = KLAST_STMT_LET,
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


#define klast_is_exprunit(type)     (type >= KLAST_EXPR_UNIT && type <= KLAST_EXPR_UNIT_END)
#define klast_is_exprpre(type)      (type >= KLAST_EXPR_PRE && type <= KLAST_EXPR_PRE_END)
#define klast_is_exprpost(type)     (type >= KLAST_EXPR_POST && type <= KLAST_EXPR_POST_END)
#define klast_is_exprbin(type)      (type == KLAST_EXPR_BIN)
#define klast_is_exprcompare(type)  (type >= KLAST_EXPR_COMPARE && type <= KLAST_EXPR_COMPARE_END)
#define klast_is_exprter(type)      (type == KLAST_EXPR_TER)
#define klast_is_stmt(type)         (type >= KLAST_STMT && type <= KLAST_STMT_END)
#define klast_is_expr(type)         (type >= KLAST_EXPR && type <= KLAST_EXPR_END)


typedef struct tagKlAstInfo KlAstInfo;


/* this serves as the base class of abstract syntax tree node,
 * and will be contained to the header of any other node.
 */
#define KL_DERIVE_FROM_KlAst(prefix)  const KlAstInfo* prefix##info; KlFileOffset prefix##begin; KlFileOffset prefix##end
typedef struct tagKlAst {
  KL_DERIVE_FROM(KlAst, );
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


/* forward declaration */
typedef struct tagKlAstExprList KlAstExprList;
typedef struct tagKlAstStmtList KlAstStmtList;

typedef struct tagKlAstClassFieldDesc {
  KlStrDesc name;
  bool shared;
} KlAstClassFieldDesc;

typedef struct tagKlAstIdentifier {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlStrDesc id;                     /* identifier */
  size_t stkid;                     /* used by code generator for pattern-match and pattern-binding */
} KlAstIdentifier;

typedef struct tagKlAstMap {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst** keys;                     /* keys */
  KlAst** vals;                     /* values */
  size_t npair;                     /* number of k-v pairs */
} KlAstMap;

typedef struct tagKlAstMapGenerator {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlStrDesc mapid;                  /* temporary identifier for array */
  KlAstStmtList* block;             /* code that generates an array */
} KlAstMapGenerator;

typedef struct tagKlAstArray {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* exprlist;             /* exprlist */
} KlAstArray;

typedef struct tagKlAstArrayGenerator {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlStrDesc arrid;                  /* temporary identifier for array */
  KlAstStmtList* block;             /* code that generates an array */
} KlAstArrayGenerator;

typedef struct tagKlAstClass {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstClassFieldDesc* fields;      /* names of fields */
  KlAst** vals;                     /* values */
  size_t nfield;
  KlAst* baseclass;
} KlAstClass;

typedef struct tagKlAstConstant {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlConstant con;
} KlAstConstant;

typedef struct tagKlAstVararg {
  KL_DERIVE_FROM(KlAst, _astbase_);
} KlAstVararg;

typedef struct tagKlAstExprList {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst** exprs;                      /* elements of exprlist */
  size_t nexpr;                       /* number of elements */
} KlAstExprList;

typedef struct tagKlAstBin {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlTokenKind op;
  KlAst* loperand;
  KlAst* roperand;
} KlAstBin;

typedef struct tagKlAstPre {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlTokenKind op;
  KlAst* operand;
} KlAstPre;

typedef struct tagKlAstNew {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* klclass;
  KlAstExprList* args;   /* parameters for new operator */
} KlAstNew;

typedef struct tagKlAstYield {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* vals;
} KlAstYield;

typedef struct tagKlAstPost {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlTokenKind op;
  KlAst* operand;
  KlAst* post;
} KlAstPost;

typedef struct tagKlAstCall {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* callable;
  KlAstExprList* args;
} KlAstCall;

typedef struct tagKlAstFunc {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstStmtList* block; /* function body */
  KlAstExprList* params;
  bool vararg;          /* has variable argument */
  bool is_method;
} KlAstFunc;

typedef struct tagKlAstDot {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* operand;
  KlStrDesc field;
} KlAstDot;

typedef struct tagKlAstDo {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstStmtList* stmtlist;
} KlAstDo;

typedef struct tagKlAstMatch {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* matchobj;
  KlAst** patterns;
  KlAst** exprs;
  size_t npattern;
} KlAstMatch;

typedef struct tagKlAstWhere {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* expr;
  KlAstStmtList* block;
} KlAstWhere;


typedef struct tagKlAstStmtList {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst** stmts;
  size_t nstmt;
} KlAstStmtList;

/* statements */
typedef struct tagKlAstStmtLet {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* lvals;        /* left values(exprlist) */
  KlAstExprList* rvals;        /* right values(must be exprlist or single value). */
} KlAstStmtLet;

typedef struct tagKlAstStmtMatch {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* matchobj;
  KlAst** patterns;
  KlAstStmtList** stmtlists;
  size_t npattern;
} KlAstStmtMatch;

typedef struct tagKlAstStmtLocalDefinition {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlFileOffset idbegin;
  KlFileOffset idend;
  KlStrDesc id;
  KlAst* expr;
} KlAstStmtLocalDefinition;

typedef struct tagKlAstStmtAssign {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* lvals;        /* left values(single value or exprlist) */
  KlAstExprList* rvals;        /* right values(single value or exprlist) */
} KlAstStmtAssign;

typedef struct tagKlAstStmtExpr {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* exprlist;
} KlAstStmtExpr;

typedef struct tagKlAstStmtIf {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* cond;
  KlAstStmtList* then_block;
  KlAstStmtList* else_block;  /* optional. no else block if NULL */
} KlAstStmtIf;

/* variable arguments for */
typedef struct tagKlAstStmtVFor {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* lvals;
  KlAstStmtList* block;
} KlAstStmtVFor;

/* integer for */
typedef struct tagKlAstStmtIFor {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* lval;
  KlAst* begin;
  KlAst* end;
  KlAst* step;    /* nil if NULL */
  KlAstStmtList* block;
} KlAstStmtIFor;

/* generic for */
typedef struct tagKlAstStmtGFor {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* lvals;
  KlAst* expr;
  KlAstStmtList* block;
} KlAstStmtGFor;

typedef struct tagKlAstStmtWhile {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAst* cond;
  KlAstStmtList* block;
} KlAstStmtWhile;

typedef struct tagKlAstStmtRepeat {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstStmtList* block;
  KlAst* cond;
} KlAstStmtRepeat;

typedef struct tagKlAstStmtReturn {
  KL_DERIVE_FROM(KlAst, _astbase_);
  KlAstExprList* retvals;  /* exprlist. */
} KlAstStmtReturn;

typedef struct tagKlAstStmtBreak {
  KL_DERIVE_FROM(KlAst, _astbase_);
} KlAstStmtBreak;

typedef struct tagKlAstStmtContinue {
  KL_DERIVE_FROM(KlAst, _astbase_);
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
KlAstMap* klast_map_create(KlAst** keys, KlAst** vals, size_t npair, KlFileOffset begin, KlFileOffset end);
KlAstMapGenerator* klast_mapgenerator_create(KlStrDesc arrid, KlAstStmtList* stmts, KlFileOffset begin, KlFileOffset end);
KlAstArray* klast_array_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end);
KlAstArrayGenerator* klast_arraygenerator_create(KlStrDesc arrid, KlAstStmtList* stmts, KlFileOffset begin, KlFileOffset end);
KlAstClass* klast_class_create(KlAstClassFieldDesc* fields, KlAst** vals, size_t nfield, KlAst* base, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_integer(KlCInt intval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_float(KlCFloat floatval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_boolean(KlCBool boolval, KlFileOffset begin, KlFileOffset end);
KlAstConstant* klast_constant_create_nil(KlFileOffset begin, KlFileOffset end);
KlAstVararg* klast_vararg_create(KlFileOffset begin, KlFileOffset end);
KlAstExprList* klast_exprlist_create(KlAst** exprs, size_t nexpr, KlFileOffset begin, KlFileOffset end);
KlAstBin* klast_bin_create(KlTokenKind op, KlAst* loperand, KlAst* roperand, KlFileOffset begin, KlFileOffset end);
KlAstPre* klast_pre_create(KlTokenKind op, KlAst* operand, KlFileOffset begin, KlFileOffset end);
KlAstNew* klast_new_create(KlAst* klclass, KlAstExprList* args, KlFileOffset begin, KlFileOffset end);
KlAstYield* klast_yield_create(KlAstExprList* vals, KlFileOffset begin, KlFileOffset end);
KlAstPost* klast_post_create(KlTokenKind op, KlAst* operand, KlAst* post, KlFileOffset begin, KlFileOffset end);
KlAstCall* klast_call_create(KlAst* callable, KlAstExprList* args, KlFileOffset begin, KlFileOffset end);
KlAstFunc* klast_func_create(KlAstStmtList* block, KlAstExprList* params, bool vararg, bool is_method, KlFileOffset begin, KlFileOffset end);
KlAstDot* klast_dot_create(KlAst* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end);
KlAstMatch* klast_match_create(KlAst* matchobj, KlAst** patterns, KlAst** exprs, size_t npattern, KlFileOffset begin, KlFileOffset end);
KlAstWhere* klast_where_create(KlAst* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);

KlAst* klast_exprlist_stealfirst_and_destroy(KlAstExprList* exprlist);
bool klast_isboolexpr(KlAst* ast);

static inline void klast_exprlist_shallow_replace(KlAstExprList* exprlist, KlAst** exprs, size_t nexpr) {
  free(exprlist->exprs);
  exprlist->exprs = exprs;
  exprlist->nexpr = nexpr;
}

/* statements */
KlAstStmtList* klast_stmtlist_create(KlAst** stmts, size_t nstmt, KlFileOffset begin, KlFileOffset end);
KlAstStmtLet* klast_stmtlet_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end);
KlAstStmtMatch* klast_stmtmatch_create(KlAst* matchobj, KlAst** patterns, KlAstStmtList** stmtlists, size_t npattern, KlFileOffset begin, KlFileOffset end);
KlAstStmtLocalDefinition* klast_stmtlocaldef_create(KlStrDesc id, KlFileOffset idbegin, KlFileOffset idend, KlAst* expr, KlFileOffset begin, KlFileOffset end);
KlAstStmtAssign* klast_stmtassign_create(KlAstExprList* lvals, KlAstExprList* rvals, KlFileOffset begin, KlFileOffset end);
KlAstStmtExpr* klast_stmtexpr_create(KlAstExprList* exprlist, KlFileOffset begin, KlFileOffset end);
KlAstStmtIf* klast_stmtif_create(KlAst* cond, KlAstStmtList* then_block, KlAstStmtList* else_block, KlFileOffset begin, KlFileOffset end);
KlAstStmtVFor* klast_stmtvfor_create(KlAstExprList* lvals, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtIFor* klast_stmtifor_create(KlAstExprList* lval, KlAst* ibegin, KlAst* iend, KlAst* istep, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtGFor* klast_stmtgfor_create(KlAstExprList* lvals, KlAst* expr, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtWhile* klast_stmtwhile_create(KlAst* cond, KlAstStmtList* block, KlFileOffset begin, KlFileOffset end);
KlAstStmtRepeat* klast_stmtrepeat_create(KlAstStmtList* block, KlAst* cond, KlFileOffset begin, KlFileOffset end);
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
