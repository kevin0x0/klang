#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H

#include "klang/include/ast/klast_base.h"
#include "klang/include/parse/klstrtab.h"
#include "klang/include/value/klvalue.h"
#include <stddef.h>

typedef struct tagKlAstExprUnit {
  KlAstBase base;
  union {
    KlStrDesc id;         /* identifier */
    struct {
      KlAstBase** elems;  /* elements of tuple */
      size_t nelem;       /* number of elements */
    } tuple;              /* tuple */
    struct {
      KlAstBase* block;   /* assignment statements */
    } map;                /* map constructor */
    struct {
      KlAstBase** exprs;  /* expressions */
      size_t nstmt;       /* number of expressions */
    } array;              /* array constructor */
    struct {
      KlType type;        /* boolean, integer, string or nil */
      union {
        KlStrDesc string; /* literal string */
        KlInt intval;
        KlBool boolval;
      };
    } literal;
  };
} KlAstExprUnit;

typedef struct tagKlAstExprBin {
  KlAstBase base;
  KlAstBase* loprand;
  KlAstBase* roprand;
} KlAstExprBin;

typedef struct tagKlAstExprPre {
  KlAstBase base;
  KlAstBase* oprand;
} KlAstExprPre;

typedef struct tagKlAstExprPost {
  KlAstBase base;
  union {
    struct {
      KlAstBase* block;     /* function body */
      KlStrDesc* params;    /* parameters */
      size_t nparam;        /* number of parameters */
    } func;                 /* is a function */
    struct {
      KlAstBase* oprand;
      KlAstBase* trailing;
    } other;
  };
} KlAstExprPost;

typedef struct tagKlAstExprTri {
  KlAstBase base;
  KlAstBase* cond;
  KlAstBase* lexpr;
  KlAstBase* rexpr;
} KlAstExprTri;

#endif
