#ifndef KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H
#define KEVCC_KLANG_INCLUDE_AST_KLCST_EXPR_H

#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klstrtab.h"
#include "klang/include/parse/kltokens.h"
#include "klang/include/value/klvalue.h"
#include <stddef.h>

typedef struct tagKlCstClassFieldDesc {
  KlStrDesc name;
  bool shared;
} KlCstClassFieldDesc;

typedef struct tagKlCstIdentifier {
  KlCst base;
  KlStrDesc id;                     /* identifier */
} KlCstIdentifier;

typedef struct tagKlCstMap {
  KlCst base;
  KlCst** keys;                     /* keys */
  KlCst** vals;                     /* values */
  size_t npair;                     /* number of k-v pairs */
} KlCstMap;

typedef struct tagKlCstArray {
  KlCst base;
  KlCst* vals;                      /* tuple */
} KlCstArray;

typedef struct tagKlCstArrayGenerator {
  KlCst base;
  KlStrDesc arrid;                  /* temporary identifier for array */
  KlCst* block;                     /* code that generates an array */
} KlCstArrayGenerator;

typedef struct tagKlCstClass {
  KlCst base;
  KlCstClassFieldDesc* fields;      /* names of fields */
  KlCst** vals;                     /* values */
  size_t nfield;
  KlCst* baseclass;
} KlCstClass;

typedef struct tagKlConstant {
  KlType type;                      /* boolean, integer, string or nil */
  union {
    KlStrDesc string;               /* literal string */
    KlInt intval;
    KlFloat floatval;
    KlBool boolval;
  };
} KlConstant;

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
  KlCst* args;            /* parameters for new operator */
} KlCstNew;

typedef struct tagKlCstYield {
  KlCst base;
  KlCst* vals;
} KlCstYield;

typedef struct tagKlCstPost {
  KlCst base;
  KlTokenKind op;
  KlCst* operand;
  KlCst* post;
} KlCstPost;

typedef struct tagKlCstFunc {
  KlCst base;
  KlCst* block;         /* function body */
  KlStrDesc* params;    /* parameters */
  uint8_t nparam;       /* number of parameters */
  bool vararg;          /* has variable argument */
  bool is_method;
} KlCstFunc;

typedef struct tagKlCstDot {
  KlCst base;
  KlCst* operand;
  KlStrDesc field;
} KlCstDot;

typedef struct tagKlCstSel {
  KlCst base;
  KlCst* cond;
  KlCst* texpr;     /* true */
  KlCst* fexpr;     /* false */
} KlCstSel;


KlCstIdentifier* klcst_id_create(KlStrDesc id, KlFileOffset begin, KlFileOffset end);
KlCstMap* klcst_map_create(KlCst** keys, KlCst** vals, size_t npair, KlFileOffset begin, KlFileOffset end);
KlCstArray* klcst_array_create(KlCst* vals, KlFileOffset begin, KlFileOffset end);
KlCstArrayGenerator* klcst_arraygenerator_create(KlStrDesc arrid, KlCst* stmts, KlFileOffset begin, KlFileOffset end);
KlCstClass* klcst_class_create(KlCstClassFieldDesc* fields, KlCst** vals, size_t nfield, KlCst* base, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_string(KlStrDesc string, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_integer(KlInt intval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_float(KlFloat floatval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_boolean(KlInt boolval, KlFileOffset begin, KlFileOffset end);
KlCstConstant* klcst_constant_create_nil(KlFileOffset begin, KlFileOffset end);
KlCstVararg* klcst_vararg_create(KlFileOffset begin, KlFileOffset end);
KlCstTuple* klcst_tuple_create(KlCst** elems, size_t nelem, KlFileOffset begin, KlFileOffset end);
KlCstBin* klcst_bin_create(KlTokenKind op, KlCst* loperand, KlCst* roperand, KlFileOffset begin, KlFileOffset end);
KlCstPre* klcst_pre_create(KlTokenKind op, KlCst* operand, KlFileOffset begin, KlFileOffset end);
KlCstNew* klcst_new_create(KlCst* klclass, KlCst* params, KlFileOffset begin, KlFileOffset end);
KlCstYield* klcst_yield_create(KlCst* vals, KlFileOffset begin, KlFileOffset end);
KlCstPost* klcst_post_create(KlTokenKind op, KlCst* operand, KlCst* post, KlFileOffset begin, KlFileOffset end);
KlCstFunc* klcst_func_create(KlCst* block, KlStrDesc* params, uint8_t nparam, bool vararg, bool is_method, KlFileOffset begin, KlFileOffset end);
KlCstDot* klcst_dot_create(KlCst* operand, KlStrDesc field, KlFileOffset begin, KlFileOffset end);
KlCstSel* klcst_sel_create(KlCst* cond, KlCst* texpr, KlCst* fexpr, KlFileOffset begin, KlFileOffset end);

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

#endif
