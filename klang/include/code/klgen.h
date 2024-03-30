#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_H

#include "klang/include/code/klcode.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include <setjmp.h>


kgarray_decl(KlCode, KlCodeArray, klcodearr, pass_ref,)
kgarray_decl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_decl(KlFilePosition, KlFPArray, klfparr, pass_val,)

typedef struct tagKlGenUnit KlGenUnit;

struct tagKlGenUnit {
  KlSymTbl* symtbl;           /* current symbol table */
  KlSymTbl* reftbl;           /* table that records references to upper klang function */
  KlConTbl* contbl;           /* constant table */
  KlSymTblPool* symtblpool;   /* object pool */
  KlCodeArray nestedfunc;     /* functions created inside this function */
  KlInstArray code;
  KlFPArray position;         /* position information (for debug) */
  KlStrTab* strtab;
  size_t stksize;             /* current used stack size */
  size_t framesize;           /* stack frame size of this klang function */
  KlGenUnit* prev;
  Ki* input;
  jmp_buf jmppos;
  KlError* klerror;
  struct {
    char* inputname;
    bool debug;
  } config;
  struct {
    KlStrDesc constructor;
  } string;
};

bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlStrTab* strtab, KlGenUnit* prev, Ki* input, KlError* klerror);
void klgen_destroy(KlGenUnit* gen);

void klgen_error(KlGenUnit* gen, KlFileOffset begin, KlFileOffset end, const char* format, ...);
KlSymbol* klgen_getsymbol(KlGenUnit* gen, KlStrDesc name);

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

static inline size_t klgen_currcodesize(KlGenUnit* gen) {
  return klinstarr_size(&gen->code);
}

static inline void klgen_popinst(KlGenUnit* gen, size_t npop) {
  klinstarr_pop_back(&gen->code, npop);
}

static inline void klgen_popinstto(KlGenUnit* gen, size_t to) {
  klgen_popinst(gen, klgen_currcodesize(gen) - to);
}

static inline KlFilePosition klgen_position(KlFileOffset begin, KlFileOffset end) {
  KlFilePosition position = { .begin = begin, .end = end };
  return position;
}

#define klgen_cstposition(cst) klgen_position(klcst_begin(cst), klcst_end(cst))



kl_noreturn static inline void klgen_error_fatal(KlGenUnit* gen, const char* message) {
  klgen_error(gen, 0, 0, message);
  longjmp(gen->jmppos, 1);
}

#define klgen_oomifnull(expr)  {                                                \
  if (kl_unlikely(!expr))                                                       \
    klgen_error_fatal(gen, "out of memory");                                    \
}

static inline size_t klgen_pushinst(KlGenUnit* gen, KlInstruction inst, KlFilePosition position) {
  size_t pc = klinstarr_size(&gen->code);
  if (kl_unlikely(!klinstarr_push_back(&gen->code, inst)))
    klgen_error_fatal(gen, "out of memory");
  if (gen->config.debug)
    klfparr_push_back(&gen->position, position);
  return pc;
}

static inline void klgen_pushinstmethod(KlGenUnit* gen, size_t obj, size_t method, size_t narg, size_t nret, KlFilePosition position) {
  klgen_pushinst(gen, klinst_method(obj, method), position);
  klgen_pushinst(gen, klinst_methodextra(narg, nret), position);
}

static inline KlCst* klgen_exprpromotion(KlCst* cst) {
  while (klcst_kind(cst) == KLCST_EXPR_TUPLE && klcast(KlCstTuple*, cst)->nelem == 1) {
    cst = klcast(KlCstTuple*, cst)->elems[0];
  }
  return cst;
}

#endif
