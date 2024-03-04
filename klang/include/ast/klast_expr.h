#ifndef KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLAST_EXPR_H

#include "klang/include/ast/klast.h"
#include "klang/include/parse/klstrtab.h"
#include "klang/include/value/klvalue.h"
#include <stddef.h>

typedef struct tagKlAstClassFieldDesc {
  KlStrDesc name;
  bool shared;
} KlAstClassFieldDesc;

typedef struct tagKlAstExprUnit {
  KlAst base;
  union {
    KlStrDesc id;                       /* identifier */
    struct {
      KlAst** elems;                    /* elements of tuple */
      size_t nelem;                     /* number of elements */
    } tuple;                            /* tuple */
    struct {
      KlAstClassFieldDesc* fields;      /* names of fields */
      KlAst** vals;                     /* values */
      size_t nfield;
    } klclass;                          /* class constructor */
    struct {
      KlAst** keys;                     /* keys */
      KlAst** vals;                     /* values */
      size_t npair;                     /* number of k-v pairs */
    } map;
    struct {
      KlAst* arrgen;                    /* code that generates an array, or a tuple(elements list) */
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
} KlAstExprUnit;

typedef struct tagKlAstExprBin {
  KlAst base;
  KlAst* loprand;
  KlAst* roprand;
} KlAstExprBin;

typedef struct tagKlAstExprPre {
  KlAst base;
  KlAst* oprand;
} KlAstExprPre;

typedef struct tagKlAstExprPost {
  KlAst base;
  union {
    struct {
      KlAst* block;         /* function body */
      KlStrDesc* params;    /* parameters */
      uint32_t nparam;      /* number of parameters */
      bool vararg;          /* has variable arguments */
    } func;
    struct {
      KlAst* oprand;
      KlAst* trailing;
    } other;
  };
} KlAstExprPost;

typedef struct tagKlAstExprTri {
  KlAst base;
  KlAst* cond;
  KlAst* lexpr;
  KlAst* rexpr;
} KlAstExprTri;

KlAstExprUnit* klast_exprunit_create(KlAstType tpye);
KlAstExprBin* klast_exprbin_create(KlAstType type);
KlAstExprPre* klast_exprpre_create(KlAstType type);
KlAstExprPost* klast_exprpost_create(KlAstType type);
KlAstExprTri* klast_exprtri_create(KlAstType type);

#endif
