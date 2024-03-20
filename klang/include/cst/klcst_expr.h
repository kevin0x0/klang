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

typedef union tagKlCstArray {
  KlCst base;
  KlCst* vals;                      /* tuple */
} KlCstArray;

typedef union tagKlCstArrayGenerator {
  KlCst base;
  KlStrDesc arrid;                  /* temporary identifier for array */
  KlCst* stmts;                     /* code that generates an array */
} KlCstArrayGenerator;

typedef struct tagKlCstClass {
  KlCst base;
  KlCstClassFieldDesc* fields;      /* names of fields */
  KlCst** vals;                     /* values */
  size_t nfield;
} KlCstClass;

typedef struct tagKlCstConstant {
  KlCst base;
  KlType type;                      /* boolean, integer, string or nil */
  union {
    KlStrDesc string;               /* literal string */
    KlInt intval;
    KlBool boolval;
  };
} KlCstConstant;

typedef struct tagKlCstVararg {
  KlCst base;
} KlCstVararg;

typedef struct tagKlCstThis {
  KlCst base;
} KlCstThis;

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
  KlCst* params;            /* parameters for new operator */
} KlCstNew;

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


KlCstIdentifier* klcst_id_create(KlStrDesc id, KlFilePos begin, KlFilePos end);
KlCstMap* klcst_map_create(KlCst** keys, KlCst** vals, size_t npair, KlFilePos begin, KlFilePos end);
KlCstArray* klcst_array_create(KlCst* vals, KlFilePos begin, KlFilePos end);
KlCstArrayGenerator* klcst_arraygenerator_create(KlStrDesc arrid, KlCst* stmts, KlFilePos begin, KlFilePos end);
KlCstClass* klcst_class_create(KlCstClassFieldDesc* fields, KlCst** vals, size_t nfield, KlFilePos begin, KlFilePos end);
KlCstConstant* klcst_constant_create_string(KlStrDesc string, KlFilePos begin, KlFilePos end);
KlCstConstant* klcst_constant_create_integer(KlInt intval, KlFilePos begin, KlFilePos end);
KlCstConstant* klcst_constant_create_boolean(KlInt boolval, KlFilePos begin, KlFilePos end);
KlCstConstant* klcst_constant_create_nil(KlFilePos begin, KlFilePos end);
KlCstVararg* klcst_vararg_create(KlFilePos begin, KlFilePos end);
KlCstThis* klcst_this_create(KlFilePos begin, KlFilePos end);
KlCstTuple* klcst_tuple_create(KlCst** elems, size_t nelem, KlFilePos begin, KlFilePos end);
KlCstBin* klcst_bin_create(KlTokenKind op, KlCst* loperand, KlCst* roperand, KlFilePos begin, KlFilePos end);
KlCstPre* klcst_pre_create(KlTokenKind op, KlCst* operand, KlFilePos begin, KlFilePos end);
KlCstNew* klcst_new_create(KlCst* klclass, KlCst* params, KlFilePos begin, KlFilePos end);
KlCstPost* klcst_post_create(KlTokenKind op, KlCst* operand, KlCst* post, KlFilePos begin, KlFilePos end);
KlCstFunc* klcst_func_create(KlCst* block, KlStrDesc* params, uint8_t nparam, bool vararg, KlFilePos begin, KlFilePos end);
KlCstDot* klcst_dot_create(KlCst* operand, KlStrDesc field, KlFilePos begin, KlFilePos end);
KlCstSel* klcst_sel_create(KlCst* cond, KlCst* texpr, KlCst* fexpr, KlFilePos begin, KlFilePos end);


static inline void klcst_tuple_shallow_replace(KlCstTuple* tuple, KlCst** elems, size_t nelem) {
  free(tuple->elems);
  tuple->elems = elems;
  tuple->nelem = nelem;
}

#endif
