#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H

#include "klang/include/cst/klcst.h"
#include "klang/include/parse/klstrtab.h"
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
      KlCst* arrgen;                    /* code that generates an array, or a tuple(elements list) */
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
  KlCst* loprand;
  KlCst* roprand;
} KlCstExprBin;

typedef struct tagKlCstExprPre {
  KlCst base;
  KlCst* oprand;
} KlCstExprPre;

typedef struct tagKlCstExprPost {
  KlCst base;
  union {
    struct {
      KlCst* block;         /* function body */
      KlStrDesc* params;    /* parameters */
      uint32_t nparam;      /* number of parameters */
      bool vararg;          /* has variable arguments */
    } func;
    struct {
      KlCst* oprand;
      KlCst* trailing;
    } other;
  };
} KlCstExprPost;

typedef struct tagKlCstExprTri {
  KlCst base;
  KlCst* cond;
  KlCst* lexpr;
  KlCst* rexpr;
} KlCstExprTri;

KlCstExprUnit* klcst_exprunit_create(KlCstType tpye);
KlCstExprBin* klcst_exprbin_create(KlCstType type);
KlCstExprPre* klcst_exprpre_create(KlCstType type);
KlCstExprPost* klcst_exprpost_create(KlCstType type);
KlCstExprTri* klcst_exprtri_create(KlCstType type);

#endif
