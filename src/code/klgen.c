#include "include/code/klgen.h"
#include "include/ast/klstrtbl.h"
#include "include/code/klcode.h"
#include "include/code/klcodeval.h"
#include "include/code/klcontbl.h"
#include "include/code/klgen_emit.h"
#include "include/code/klgen_stmt.h"
#include "include/code/klsymtbl.h"
#include "include/ast/klast.h"

kgarray_impl(KlCode*, KlCodeArray, klcodearr, pass_val,)
kgarray_impl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_impl(KlFilePosition, KlFPArray, klfparr, pass_val,)

#define KLGEN_NSUBFUNC  (klbit(16))
#define KLGEN_NREG      (klbit(8) - 1)
#define KLGEN_NCONST    (klbit(16))
#define KLGEN_NREF      (klbit(16))

void klgen_validate(KlGenUnit* gen) {
  if (klcodearr_size(&gen->subfunc) >= KLGEN_NSUBFUNC)
    klgen_error_fatal(gen, "too many functions defined in a function");
  if (klcontbl_size(gen->contbl) >= KLGEN_NCONST)
    klgen_error_fatal(gen, "too many constants");
  klgen_stackfree(gen, 0);
  if (gen->framesize >= KLGEN_NREG)
    klgen_error_fatal(gen, "too many registers used");
  if (klsymtbl_size(gen->reftbl) >= KLGEN_NREF)
    klgen_error_fatal(gen, "refereces too many variables from upper function");
}

bool klgen_init_commonstrings(KlStrTbl* strtbl, KlGUCommonString* strings) {
  memset(strings, 0, sizeof (KlGUCommonString));
#define newstring(member, str) {                                                \
  char* _str = klstrtbl_newstring(strtbl, str);                                 \
  if (kl_unlikely(!_str)) {                                                     \
    return false;                                                               \
  } else {                                                                      \
    strings->member.id = klstrtbl_stringid(strtbl, _str);                       \
    strings->member.length = sizeof (str) - 1;                                  \
  }                                                                             \
}

  newstring(pattern, "pattern ");
  newstring(pattern_add, "pattern +");
  newstring(pattern_sub, "pattern -");
  newstring(pattern_mul, "pattern *");
  newstring(pattern_div, "pattern /");
  newstring(pattern_idiv, "pattern //");
  newstring(pattern_mod, "pattern %");
  newstring(pattern_neg, "pattern u-");
  newstring(pattern_concat, "pattern ..");
  newstring(itermethod, "<-");
  newstring(init, "init");
  return true;
#undef newstring
}

bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlGUCommonString* strings, KlStrTbl* strtbl, KlGenUnit* prev, KlCodeGenConfig* config) {
  if (kl_unlikely(!(gen->symtbl = klsymtblpool_alloc(symtblpool, strtbl, NULL)))) {
    return false;
  }
  gen->reftbl = gen->symtbl;
  if (kl_unlikely(!(gen->contbl = klcontbl_create(8, strtbl)))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    return false;
  }
  gen->symtblpool = symtblpool;
  if (kl_unlikely(!klcodearr_init(&gen->subfunc, 2))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klcontbl_delete(gen->contbl);
    return false;
  }
  if (kl_unlikely(!klinstarr_init(&gen->code, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->subfunc);
    return false;
  }
  if (kl_unlikely(!klfparr_init(&gen->position, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->subfunc);
    klinstarr_destroy(&gen->code);
    return false;
  }
  gen->strtbl = strtbl;
  gen->strings = strings;
  gen->stksize = 0;
  gen->framesize = 0;
  gen->jmpinfo.jumpinfo = NULL;
  gen->jmpinfo.breaklist = NULL;
  gen->jmpinfo.continuelist = NULL;
  gen->jmpinfo.break_scope = NULL;
  gen->jmpinfo.continue_scope = NULL;
  gen->jmpinfo.isjmptarget = false;
  gen->prev = prev;
  gen->config = config;
  return true;
}

void klgen_destroy(KlGenUnit* gen) {
  KlSymTbl* symtbl = gen->symtbl;
  while (symtbl) {
    KlSymTbl* parent = klsymtbl_parent(symtbl);
    klsymtblpool_dealloc(gen->symtblpool, symtbl);
    symtbl = parent;
  }
  klcontbl_delete(gen->contbl);
  for (size_t i = 0; i < klcodearr_size(&gen->subfunc); ++i) {
    KlCode** code = klcodearr_access(&gen->subfunc, i);
    klcode_delete(*code);
  }
  klfparr_destroy(&gen->position);
  klcodearr_destroy(&gen->subfunc);
  klinstarr_destroy(&gen->code);
}

KlCode* klgen_tocode_and_destroy(KlGenUnit* gen, unsigned nparam) {
  klinstarr_shrink(&gen->code);
  klfparr_shrink(&gen->position);
  klcodearr_shrink(&gen->subfunc);
  KlConstant* constants = (KlConstant*)malloc(klcontbl_size(gen->contbl) * sizeof (KlConstant));
  KlCRefInfo* refinfo = (KlCRefInfo*)malloc(klsymtbl_size(gen->reftbl) * sizeof (KlCRefInfo));
  if (kl_unlikely(!constants || !refinfo)) {
    free(constants);
    free(refinfo);
    klgen_error_fatal(gen, "out of memory");
  }
  klcontbl_setconstants(gen->contbl, constants);
  klreftbl_setrefinfo(gen->reftbl, refinfo);
  size_t codelen = klinstarr_size(&gen->code);
  KlInstruction* insts = klinstarr_steal(&gen->code);
  KlFilePosition* posinfo = gen->config->posinfo ? klfparr_steal(&gen->position) : (klfparr_destroy(&gen->position), NULL);
  size_t nnested = klcodearr_size(&gen->subfunc);
  KlCode** nestedfunc = klcodearr_steal(&gen->subfunc);

  klgen_stackfree(gen, 0);
  KlCode* code = klcode_create(refinfo, klsymtbl_size(gen->reftbl), constants, klcontbl_size(gen->contbl),
                               insts, posinfo, codelen, nestedfunc, nnested,
                               gen->strtbl, nparam, gen->framesize, gen->config->srcfile);
  kl_assert(gen->symtbl == gen->reftbl, "");
  klsymtblpool_dealloc(gen->symtblpool, gen->symtbl);
  klcontbl_delete(gen->contbl);
  if (kl_unlikely(!code)) {
    for (size_t i = 0; i < nnested; ++i)
      klcode_delete(nestedfunc[i]);
    free(nestedfunc);
    free(insts);
    free(posinfo);
    free(refinfo);
    free(constants);
    return NULL;
  }
  return code;
}

KlSymbol* klgen_getsymbolref(KlGenUnit* gen, KlStrDesc name) {
  if (!gen) return NULL;
  KlSymbol* symbol;
  KlSymTbl* symtbl = gen->symtbl;
  while (symtbl) {
    symbol = klsymtbl_search(symtbl, name);
    if (symbol) {
      symtbl->info.referenced = true;
      return symbol;
    }
    symtbl = klsymtbl_parent(symtbl);
  }
  KlSymbol* refsymbol = klgen_getsymbolref(gen->prev, name);
  if (!refsymbol) return NULL;  /* is global variable */
  symbol = klsymtbl_insert(gen->reftbl, name);
  klgen_oomifnull(gen, symbol);
  symbol->attr.kind = KLVAL_REF;
  symbol->attr.idx = klsymtbl_size(gen->reftbl) - 1;
  symbol->attr.refto = refsymbol;
  return symbol;
}

KlSymbol* klgen_newsymbol(KlGenUnit* gen, KlStrDesc name, KlCIdx idx, KlFilePosition symbolpos) {
  KlSymTbl* symtbl = gen->symtbl;
  KlSymbol* symbol = klsymtbl_search(symtbl, name);
  if (kl_unlikely(symbol)) {
    klgen_error(gen, symbolpos.begin, symbolpos.end,
                "redefinition of symbol: %.*s",
                symbol->name.length,
                klstrtbl_getstring(gen->strtbl, symbol->name.id));
    symbol->attr.idx = idx;
    return symbol;
  }
  symbol = klsymtbl_insert(symtbl, name);
  if (kl_unlikely(!symbol))
    klgen_error_fatal(gen, "out of memory");
  symbol->attr.kind = KLVAL_STACK;
  symbol->attr.idx = idx;
  return symbol;
}

KlSymbol* klgen_getsymbol(KlGenUnit* gen, KlStrDesc name) {
  if (!gen) return NULL;
  KlSymbol* symbol;
  KlSymTbl* symtbl = gen->symtbl;
  while (symtbl) {
    symbol = klsymtbl_search(symtbl, name);
    if (symbol) return symbol;
    symtbl = klsymtbl_parent(symtbl);
  }
  KlSymbol* refsymbol = klgen_getsymbolref(gen->prev, name);
  if (!refsymbol) return NULL;  /* is global variable */
  symbol = klsymtbl_insert(gen->reftbl, name);
  klgen_oomifnull(gen, symbol);
  symbol->attr.kind = KLVAL_REF;
  symbol->attr.idx = klsymtbl_size(gen->reftbl) - 1;
  symbol->attr.refto = refsymbol;
  return symbol;
}

void klgen_pushsymtbl(KlGenUnit* gen) {
  KlSymTbl* symtbl = klsymtblpool_alloc(gen->symtblpool, gen->strtbl, gen->symtbl);
  klgen_oomifnull(gen, symtbl);
  gen->symtbl = symtbl;
  symtbl->info.stkbase = klgen_stacktop(gen);
}

void klgen_popsymtbl(KlGenUnit* gen) {
  KlSymTbl* symtbl = gen->symtbl;
  gen->symtbl = klsymtbl_parent(symtbl);
  klsymtblpool_dealloc(gen->symtblpool, symtbl);
}

KlCode* klgen_file(KlAstStmtList* ast, KlStrTbl* strtbl, KlCodeGenConfig* config) {
  /* create genunit */
  KlGUCommonString strings;
  if (kl_unlikely(!klgen_init_commonstrings(strtbl, &strings))) {
    klerror_error(config->klerr, config->input, config->inputname, 0, 0, "out of memory");
    return NULL;
  }
  KlSymTblPool symtblpool;
  klsymtblpool_init(&symtblpool);
  KlGenUnit gen;
  if (kl_unlikely(!klgen_init(&gen, &symtblpool, &strings, strtbl, NULL, config))) {
    klsymtblpool_destroy(&symtblpool);
    klerror_error(config->klerr, config->input, config->inputname, 0, 0, "out of memory");
    return NULL;
  }
  gen.vararg = true;
  if (setjmp(gen.jmppos) == 0) {
    /* begin a new scope */
    klgen_pushsymtbl(&gen);
    /* handle variable arguments */
    klgen_emit(&gen, klinst_adjustargs(), klgen_position(klast_begin(ast), klast_begin(ast)));

    /* generate code for function body */
    kl_assert(klast_kind(ast) == KLAST_STMT_BLOCK, "");
    klgen_stmtlist(&gen, ast);
    /* add a return statement if 'return' is missing */
    if (!klast_mustreturn(ast)) {
      if (gen.symtbl->info.referenced)
        klgen_emit(&gen, klinst_close(0), klgen_position(klast_end(ast), klast_end(ast)));
      klgen_emit(&gen, klinst_return0(), klgen_position(klast_end(ast), klast_end(ast)));
    }
    /* close the scope */
    klgen_popsymtbl(&gen);

    klgen_validate(&gen);
    /* code generation is done */
    /* convert the 'newgen' to KlCode */
    KlCode* funccode = klgen_tocode_and_destroy(&gen, 0);
    klsymtblpool_destroy(&symtblpool);
    if (kl_unlikely(!funccode)) {
      klerror_error(config->klerr, config->input, config->inputname, 0, 0, "out of memory");
      return NULL;
    }
    return funccode;
  } else {
    klgen_destroy(&gen);
    klsymtblpool_destroy(&symtblpool);
    return NULL;
  }
}


void klgen_error(KlGenUnit* gen, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  va_list args;
  va_start(args, format);
  klerror_errorv(gen->config->klerr, gen->config->input, gen->config->inputname, begin, end, format, args);
  va_end(args);
}

kl_noreturn void klgen_error_fatal(KlGenUnit* gen, const char* message) {
  klgen_error(gen, 0, 0, message);
  longjmp(gen->jmppos, 1);
}
