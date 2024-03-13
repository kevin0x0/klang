#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H

#include "klang/include/cst/klcst.h"
#include "klang/include/parse/klstrtab.h"
#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include <stddef.h>

typedef struct tagKlCstClassFieldDesc {
  KlStrDesc name;
  bool shared;
} KlCstClassFieldDesc;

typedef struct tagKlCstExprUnit {
  KlCst base;
  union {
    KlStrDesc id;                       /* identifier */
    struct {
      KlCst** elems;                    /* elements of tuple */
      size_t nelem;                     /* number of elements */
    } tuple;                            /* tuple */
    struct {
      KlCstClassFieldDesc* fields;      /* names of fields */
      KlCst** vals;                     /* values */
      size_t nfield;
    } klclass;                          /* class constructor */
    struct {
      KlCst** keys;                     /* keys */
      KlCst** vals;                     /* values */
      size_t npair;                     /* number of k-v pairs */
    } map;
    struct {
      KlCst* exprs;                     /* expressions */
      KlCst* stmts;                     /* code that generates an array */
    } array;                            /* array constructor */
    struct {
      KlType type;                      /* boolean, integer, string or nil */
      union {
        KlStrDesc string;               /* literal string */
        KlInt intval;
        KlBool boolval;
      };
    } literal;
  };
} KlCstExprUnit;

typedef struct tagKlCstExprBin {
  KlCst base;
  KlTokenKind binop;
  KlCst* loperand;
  KlCst* roperand;
} KlCstExprBin;

typedef struct tagKlCstExprPre {
  KlCst base;
  KlTokenKind op;
  KlCst* operand;
  KlCst* params;            /* parameters for new operator */
} KlCstExprPre;

typedef struct tagKlCstExprPost {
  KlCst base;
  union {
    struct {
      KlCst* block;         /* function body */
      KlStrDesc* params;    /* parameters */
      uint32_t nparam;      /* number of parameters */
      bool vararg;          /* has variable argument */
    } func;
    struct {
      KlCst* indexable;
      KlCst* index;
    } index;
    struct {
      KlCst* callable;
      KlCst* param;
    } call;
    struct  {
      KlCst* operand;
      KlStrDesc field;
    } dot;
  };
} KlCstExprPost;

typedef struct tagKlCstExprTer {
  KlCst base;
  KlCst* cond;
  KlCst* lexpr;
  KlCst* rexpr;
} KlCstExprTer;

KlCstExprUnit* klcst_exprunit_create(KlCstKind tpye);
KlCstExprBin* klcst_exprbin_create(KlTokenKind op);
KlCstExprPre* klcst_exprpre_create(KlTokenKind op);
KlCstExprPost* klcst_exprpost_create(KlCstKind type);
KlCstExprTer* klcst_exprter_create(void);

void klcst_expr_tuple_delete_after_stolen(KlCst* tuple);

#endif
