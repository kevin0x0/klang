#ifndef _KLANG_INCLUDE_CODE_KLGEN_H_
#define _KLANG_INCLUDE_CODE_KLGEN_H_

#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klcontbl.h"
#include "include/code/klsymtbl.h"
#include "include/ast/klast.h"
#include "include/ast/klstrtbl.h"
#include "deps/k/include/array/kgarray.h"
#include <setjmp.h>


kgarray_decl(KlCode*, KlCodeArray, klcodearr, pass_val, nonstatic);
kgarray_decl(KlInstruction, KlInstArray, klinstarr, pass_val, nonstatic);
kgarray_decl(KlFilePosition, KlFPArray, klfparr, pass_val, nonstatic);


typedef struct tagKlGenJumpInfo KlGenJumpInfo;
/* this struct used for evaluating boolean expression as a value. */
struct tagKlGenJumpInfo {
  KlCodeVal terminatelist;
  KlCodeVal truelist;
  KlCodeVal falselist;
  KlGenJumpInfo* prev;
};

typedef struct tagKlGUCommonString {
  KlStrDesc init;
  KlStrDesc itermethod;
  KlStrDesc pattern_add;
  KlStrDesc pattern_sub;
  KlStrDesc pattern_mul;
  KlStrDesc pattern_div;
  KlStrDesc pattern_idiv;
  KlStrDesc pattern_mod;
  KlStrDesc pattern_concat;
  KlStrDesc pattern_neg;
  KlStrDesc pattern;
} KlGUCommonString;

typedef struct tagKlGenUnit KlGenUnit;
struct tagKlGenUnit {
  KlSymTbl* symtbl;           /* current symbol table */
  KlSymTbl* reftbl;           /* table that records references to upper klang function */
  KlConTbl* contbl;           /* constant table */
  KlSymTblPool* symtblpool;   /* object pool */
  KlCodeArray subfunc;        /* functions created inside this function */
  KlInstArray code;
  KlFPArray position;         /* position information (for debug) */
  KlStrTbl* strtbl;
  KlCStkId stksize;           /* current used stack size */
  KlCStkId framesize;         /* stack frame size of this klang function */
  bool vararg;                /* has variable arguments */
  struct {
    KlGenJumpInfo* jumpinfo;  /* information needed by code generator that evaluates boolean expression as a single value */
    KlCodeVal* continuelist;  /* continue jmplist. continue is not allowed if NULL */
    KlSymTbl* continue_scope; /* the scope that start a scope that allows continue */
    KlCodeVal* breaklist;     /* break jmplist. break is not allowed if NULL */
    KlSymTbl* break_scope;    /* the scope that start a scope that allows break */
    bool isjmptarget;         /* current instruction is jump target of some instruction */
  } jmpinfo;
  KlGenUnit* prev;
  jmp_buf jmppos;
  KlCodeGenConfig* config;
  KlGUCommonString* strings;
};

bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlGUCommonString* strings, KlStrTbl* strtbl, KlGenUnit* prev, KlCodeGenConfig* config);
void klgen_destroy(KlGenUnit* gen);

bool klgen_init_commonstrings(KlStrTbl* strtbl, KlGUCommonString* strings);

KlCode* klgen_file(KlAstStmtList* ast, KlStrTbl* strtbl, KlCodeGenConfig* config);
/* check range */
void klgen_validate(KlGenUnit* gen);
/* convert to KlCode and destroy self.
 * if failed, return NULL, and destroy self. */
KlCode* klgen_tocode_and_destroy(KlGenUnit* gen, unsigned nparam);
void klgen_error(KlGenUnit* gen, KlFileOffset begin, KlFileOffset end, const char* format, ...);
KlSymbol* klgen_newsymbol(KlGenUnit* gen, KlStrDesc name, KlCStkId idx, KlFilePosition symbolpos);
KlSymbol* klgen_getsymbol(KlGenUnit* gen, KlStrDesc name);
void klgen_pushsymtbl(KlGenUnit* gen);
void klgen_popsymtbl(KlGenUnit* gen);

static inline KlCIdx klgen_newconstant(KlGenUnit* gen, KlConstant* constant);
static inline KlCIdx klgen_newstring(KlGenUnit* gen, KlStrDesc str);
static inline KlCIdx klgen_newfloat(KlGenUnit* gen, KlCFloat val);
static inline KlCIdx klgen_newinteger(KlGenUnit* gen, KlCInt val);
static inline KlConEntry* klgen_searchinteger(const KlGenUnit* gen, KlCInt val);


/* mark current pc a potential target of some jump instruction */
static inline void klgen_markjmptarget(KlGenUnit* gen);
/* unmark current pc a potential target of some jump instruction */
static inline void klgen_unmarkjmptarget(KlGenUnit* gen);
/* get current pc as target of some instructions, this will mark current pc is a jmppos */
static inline KlCPC klgen_getjmptarget(KlGenUnit* gen);


kl_noreturn void klgen_error_fatal(KlGenUnit* gen, const char* message);

#define klgen_oomifnull(gen, expr)  {                                           \
  if (kl_unlikely(!expr))                                                       \
    klgen_error_fatal(gen, "out of memory");                                    \
}

static inline KlCIdx klgen_newconstant(KlGenUnit* gen, KlConstant* constant) {
  KlConEntry* conent = klcontbl_get(gen->contbl, constant);
  klgen_oomifnull(gen, conent);
  return conent->index;
}

static inline KlCIdx klgen_newstring(KlGenUnit* gen, KlStrDesc str) {
  return klgen_newconstant(gen, &(KlConstant) { .type = KLC_STRING, .string = str });
}

static inline KlCIdx klgen_newfloat(KlGenUnit* gen, KlCFloat val) {
  return klgen_newconstant(gen, &(KlConstant) { .type = KLC_FLOAT, .floatval = val });
}

static inline KlCIdx klgen_newinteger(KlGenUnit* gen, KlCInt val) {
  return klgen_newconstant(gen, &(KlConstant) { .type = KLC_INT, .intval = val });
}

static inline KlConEntry* klgen_searchinteger(const KlGenUnit* gen, KlCInt val) {
  return klcontbl_search(gen->contbl, &(KlConstant){ .type = KLC_INT, .intval = val });
}

static inline KlCStkId klgen_stacktop(KlGenUnit* gen) {
  return gen->stksize;
}

static inline KlCStkId klgen_stackalloc(KlGenUnit* gen, size_t size) {
  KlCStkId base = gen->stksize;
  gen->stksize += size;
  return base;
}

static inline KlCStkId klgen_stackalloc1(KlGenUnit* gen) {
  return klgen_stackalloc(gen, 1);
}

static inline void klgen_stackfree(KlGenUnit* gen, KlCStkId stkid) {
  if (gen->stksize > gen->framesize)
    gen->framesize = gen->stksize;
  gen->stksize = stkid;
}

static inline KlCPC klgen_currentpc(const KlGenUnit* gen) {
  return klinstarr_size(&gen->code);
}

static inline KlFilePosition klgen_position(KlFileOffset begin, KlFileOffset end) {
  return (KlFilePosition) { .begin = begin, .end = end };
}

#define klgen_astposition(ast) klgen_position(klast_begin(ast), klast_end(ast))


static inline void klgen_markjmptarget(KlGenUnit* gen) {
  gen->jmpinfo.isjmptarget = true;
}

static inline void klgen_unmarkjmptarget(KlGenUnit* gen) {
  gen->jmpinfo.isjmptarget = false;
}

static inline KlCPC klgen_getjmptarget(KlGenUnit* gen) {
  klgen_markjmptarget(gen);
  return klgen_currentpc(gen);
}

#endif
