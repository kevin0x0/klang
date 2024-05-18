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


kgarray_decl(KlCode*, KlCodeArray, klcodearr, pass_val,)
kgarray_decl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_decl(KlFilePosition, KlFPArray, klfparr, pass_val,)


typedef struct tagKlGenJumpInfo KlGenJumpInfo;
/* this struct used for evaluating boolean expression as a value. */
struct tagKlGenJumpInfo {
  KlCodeVal terminatelist;
  KlCodeVal truelist;
  KlCodeVal falselist;
  KlGenJumpInfo* prev;
};

typedef struct tagKlGUCommonString {
  KlStrDesc constructor;
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
  KlCodeArray subfunc;     /* functions created inside this function */
  KlInstArray code;
  KlFPArray position;         /* position information (for debug) */
  KlStrTbl* strtbl;
  KlCStkId stksize;           /* current used stack size */
  KlCStkId framesize;         /* stack frame size of this klang function */
  bool vararg;                /* has variable arguments */
  struct {
    KlGenJumpInfo* jumpinfo;  /* information needed by code generator that evaluates boolean expression as a single value */
    KlCodeVal* continuelist;   /* continue jmplist. continue is not allowed if NULL */
    KlSymTbl* continue_scope; /* the scope that start a scope that allows continue */
    KlCodeVal* breaklist;      /* break jmplist. break is not allowed if NULL */
    KlSymTbl* break_scope;    /* the scope that start a scope that allows break */
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
static inline KlConEntry* klgen_searchinteger(KlGenUnit* gen, KlCInt val);


void klgen_loadval(KlGenUnit* gen, KlCStkId target, KlCodeVal val, KlFilePosition position);
static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);
static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);
static inline KlCPC klgen_emit(KlGenUnit* gen, KlInstruction inst, KlFilePosition position);
void klgen_emitloadnils(KlGenUnit* gen, KlCStkId target, size_t nnil, KlFilePosition position);
void klgen_emitmove(KlGenUnit* gen, KlCStkId target, KlCStkId from, size_t nval, KlFilePosition position);
void klgen_emitmethod(KlGenUnit* gen, KlCStkId obj, KlCStkId method, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position);
void klgen_emitcall(KlGenUnit* gen, KlCStkId callable, size_t narg, size_t nret, KlCStkId retpos, KlFilePosition position);


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

static inline KlConEntry* klgen_searchinteger(KlGenUnit* gen, KlCInt val) {
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

static inline void klgen_stackpreserve(KlGenUnit* gen, KlCStkId stkid) {
  if (gen->stksize <= stkid)
    gen->stksize = stkid + 1;
}

static inline KlCPC klgen_currentpc(KlGenUnit* gen) {
  return klinstarr_size(&gen->code);
}

static inline void klgen_popinst(KlGenUnit* gen, size_t npop) {
  klinstarr_pop_back(&gen->code, npop);
  if (gen->config->posinfo)
    klfparr_pop_back(&gen->position, npop);
}

static inline void klgen_popinstto(KlGenUnit* gen, KlCPC to) {
  klgen_popinst(gen, klgen_currentpc(gen) - to);
}

static inline KlFilePosition klgen_position(KlFileOffset begin, KlFileOffset end) {
  KlFilePosition position = { .begin = begin, .end = end };
  return position;
}

#define klgen_astposition(ast) klgen_position(klast_begin(ast), klast_end(ast))


static inline KlCPC klgen_emit(KlGenUnit* gen, KlInstruction inst, KlFilePosition position) {
  KlCPC pc = klinstarr_size(&gen->code);
  if (kl_unlikely(!klinstarr_push_back(&gen->code, inst)))
    klgen_error_fatal(gen, "out of memory");
  if (gen->config->posinfo)
    klfparr_push_back(&gen->position, position);
  return pc;
}

static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  if (val->kind == KLVAL_STACK) return;
  KlCStkId stkid = klgen_stackalloc1(gen);
  klgen_loadval(gen, stkid, *val, position);
  val->kind = KLVAL_STACK;
  val->index = stkid;
}

static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  KlCStkId stkid = klgen_stacktop(gen);
  klgen_putonstack(gen, val, position);
  if (val->index != stkid)
    klgen_emit(gen, klinst_move(stkid, val->index), position);
  klgen_stackfree(gen, stkid + 1);
}

#endif
