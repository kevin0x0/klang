#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_H

#include "klang/include/code/klcode.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/cst/klstrtbl.h"
#include "klang/include/vm/klinst.h"
#include <setjmp.h>


kgarray_decl(KlCode*, KlCodeArray, klcodearr, pass_val,)
kgarray_decl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_decl(KlFilePosition, KlFPArray, klfparr, pass_val,)


typedef struct tagKlGenJumpInfo KlGenJumpInfo;
struct tagKlGenJumpInfo {
  KlCodeVal terminatelist;
  KlCodeVal truelist;
  KlCodeVal falselist;
  KlGenJumpInfo* prev;
};

typedef struct tagKlGenBlockInfo KlGenBlockInfo;
struct tagKlGenBlockInfo {
  KlGenBlockInfo* prev;
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
  size_t stksize;             /* current used stack size */
  size_t framesize;           /* stack frame size of this klang function */
  bool vararg;                /* has variable arguments */
  struct {
    KlGenJumpInfo* jumpinfo;  /* information needed by code generator that evaluates boolean expression as a single value */
    KlCodeVal* continuejmp;
    KlSymTbl* continue_scope;
    KlCodeVal* breakjmp;
    KlSymTbl* break_scope;
  } info;
  KlGenUnit* prev;
  jmp_buf jmppos;
  KlError* klerror;
  Ki* input;
  struct {
    const char* inputname;
    bool debug;
  } config;
  KlGUCommonString* strings;
};

bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlGUCommonString* strings, KlStrTbl* strtbl, KlGenUnit* prev, Ki* input, KlError* klerror);
void klgen_destroy(KlGenUnit* gen);

bool klgen_init_commonstrings(KlStrTbl* strtbl, KlGUCommonString* strings);

KlCode* klgen_file(KlCst* cst, KlStrTbl* strtbl, Ki* input, const char* inputname, KlError* klerr, bool debug);
/* check range */
void klgen_validate(KlGenUnit* gen);
/* convert to KlCode and destroy self.
 * if failed, return NULL, and destroy self. */
KlCode* klgen_tocode_and_destroy(KlGenUnit* gen, size_t nparam);
void klgen_error(KlGenUnit* gen, KlFileOffset begin, KlFileOffset end, const char* format, ...);
KlSymbol* klgen_newsymbol(KlGenUnit* gen, KlStrDesc name, size_t idx, KlFilePosition symbolpos);
KlSymbol* klgen_getsymbol(KlGenUnit* gen, KlStrDesc name);
void klgen_pushsymtbl(KlGenUnit* gen);
void klgen_popsymtbl(KlGenUnit* gen);

static inline size_t klgen_newstring(KlGenUnit* gen, KlStrDesc str);
static inline size_t klgen_newfloat(KlGenUnit* gen, KlFloat val);
static inline size_t klgen_newinteger(KlGenUnit* gen, KlInt val);


void klgen_loadval(KlGenUnit* gen, size_t target, KlCodeVal val, KlFilePosition position);
static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);
static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position);
void klgen_emitloadnils(KlGenUnit* gen, size_t target, size_t nnil, KlFilePosition position);
void klgen_emitmove(KlGenUnit* gen, size_t target, size_t from, size_t nval, KlFilePosition position);


kl_noreturn static inline void klgen_error_fatal(KlGenUnit* gen, const char* message) {
  klgen_error(gen, 0, 0, message);
  longjmp(gen->jmppos, 1);
}

#define klgen_oomifnull(gen, expr)  {                                           \
  if (kl_unlikely(!expr))                                                       \
    klgen_error_fatal(gen, "out of memory");                                    \
}

static inline size_t klgen_newstring(KlGenUnit* gen, KlStrDesc str) {
  KlConstant constant = { .type = KL_STRING, .string = str };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  return conent->index;
}

static inline size_t klgen_newfloat(KlGenUnit* gen, KlFloat val) {
  KlConstant constant = { .type = KL_FLOAT, .floatval = val };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  return conent->index;
}

static inline size_t klgen_newinteger(KlGenUnit* gen, KlInt val) {
  KlConstant constant = { .type = KL_INT, .intval = val };
  KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
  klgen_oomifnull(gen, conent);
  return conent->index;
}

static inline size_t klgen_newconstant(KlGenUnit* gen, KlConstant* constant) {
  KlConEntry* conent = klcontbl_get(gen->contbl, constant);
  klgen_oomifnull(gen, conent);
  return conent->index;
}

static inline size_t klgen_stacktop(KlGenUnit* gen) {
  return gen->stksize;
}

static inline size_t klgen_stackalloc(KlGenUnit* gen, size_t size) {
  size_t base = gen->stksize;
  gen->stksize += size;
  return base;
}

static inline size_t klgen_stackalloc1(KlGenUnit* gen) {
  return klgen_stackalloc(gen, 1);
}

static inline void klgen_stackfree(KlGenUnit* gen, size_t stkid) {
  if (gen->stksize > gen->framesize)
    gen->framesize = gen->stksize;
  gen->stksize = stkid;
}

static inline void klgen_stackpreserve(KlGenUnit* gen, size_t stkid) {
  if (gen->stksize <= stkid)
    gen->stksize = stkid + 1;
}

static inline size_t klgen_currcodesize(KlGenUnit* gen) {
  return klinstarr_size(&gen->code);
}

static inline void klgen_popinst(KlGenUnit* gen, size_t npop) {
  klinstarr_pop_back(&gen->code, npop);
  if (gen->config.debug)
    klfparr_pop_back(&gen->position, npop);
}

static inline void klgen_popinstto(KlGenUnit* gen, size_t to) {
  klgen_popinst(gen, klgen_currcodesize(gen) - to);
}

static inline KlFilePosition klgen_position(KlFileOffset begin, KlFileOffset end) {
  KlFilePosition position = { .begin = begin, .end = end };
  return position;
}

#define klgen_cstposition(cst) klgen_position(klcst_begin(cst), klcst_end(cst))


static inline size_t klgen_emit(KlGenUnit* gen, KlInstruction inst, KlFilePosition position) {
  size_t pc = klinstarr_size(&gen->code);
  if (kl_unlikely(!klinstarr_push_back(&gen->code, inst)))
    klgen_error_fatal(gen, "out of memory");
  if (gen->config.debug)
    klfparr_push_back(&gen->position, position);
  return pc;
}

static inline void klgen_emitmethod(KlGenUnit* gen, size_t obj, size_t method, size_t narg, size_t nret, size_t retpos, KlFilePosition position) {
  klgen_emit(gen, klinst_method(obj, method), position);
  klgen_emit(gen, klinst_methodextra(narg, nret, retpos), position);
}

static inline void klgen_emitcall(KlGenUnit* gen, size_t callable, size_t narg, size_t nret, size_t retpos, KlFilePosition position) {
  klgen_emit(gen, klinst_call(callable), position);
  klgen_emit(gen, klinst_callextra(narg, nret, retpos), position);
}

static inline void klgen_putonstack(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  if (val->kind == KLVAL_STACK) return;
  size_t stkid = klgen_stackalloc1(gen);
  klgen_loadval(gen, stkid, *val, position);
  val->kind = KLVAL_STACK;
  val->index = stkid;
}

static inline void klgen_putonstktop(KlGenUnit* gen, KlCodeVal* val, KlFilePosition position) {
  size_t stkid = klgen_stacktop(gen);
  klgen_putonstack(gen, val, position);
  if (val->index != stkid)
    klgen_emit(gen, klinst_move(stkid, val->index), position);
  klgen_stackfree(gen, stkid + 1);
}

#endif
