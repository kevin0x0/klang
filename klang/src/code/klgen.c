#include "klang/include/code/klgen.h"
#include "klang/include/code/klsymtbl.h"
#include <string.h>

kgarray_impl(KlCode*, KlCodeArray, klcodearr, pass_val,)
kgarray_impl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_impl(KlFilePosition, KlFPArray, klfparr, pass_val,)


bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlStrTab* strtab, KlGenUnit* prev, Ki* input, KlError* klerror) {
  if (kl_unlikely(!(gen->symtbl = klsymtblpool_alloc(symtblpool, strtab, NULL)))) {
    return false;
  }
  gen->reftbl = gen->symtbl;
  if (kl_unlikely(gen->contbl = klcontbl_create(8, strtab))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    return false;
  }
  gen->symtblpool = symtblpool;
  if (kl_unlikely(!klcodearr_init(&gen->nestedfunc, 2))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    return false;
  }
  if (kl_unlikely(!klinstarr_init(&gen->code, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->nestedfunc);
    return false;
  }
  if (kl_unlikely(!klfparr_init(&gen->position, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->nestedfunc);
    klinstarr_destroy(&gen->code);
    return false;
  }
  gen->strtab = strtab;
  gen->stksize = 0;
  gen->framesize = 0;
  gen->info.jumpinfo = NULL;
  gen->info.breakjmp = NULL;
  gen->info.continuejmp = NULL;
  gen->info.break_scope = NULL;
  gen->info.continue_scope = NULL;
  gen->prev = prev;
  gen->klerror = klerror;

  gen->input = input;
  if (prev) {
    gen->config = prev->config;
  } else {
    gen->config.inputname = "unnamed";
    gen->config.debug = false;
  }


  char* constructor = klstrtab_newstring(strtab, "constructor");
  char* itermethod = klstrtab_newstring(strtab, "<-");
  if (kl_unlikely(!constructor || !itermethod)) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->nestedfunc);
    klinstarr_destroy(&gen->code);
    klfparr_destroy(&gen->position);
    return false;
  }
  gen->string.constructor.id = klstrtab_stringid(gen->strtab, constructor);
  gen->string.constructor.length = strlen("constructor");
  gen->string.itermethod.id = klstrtab_stringid(gen->strtab, itermethod);
  gen->string.itermethod.length = strlen("<-");
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
  klcodearr_destroy(&gen->nestedfunc);
  klinstarr_destroy(&gen->code);
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

KlSymbol* klgen_newsymbol(KlGenUnit* gen, KlStrDesc name, size_t idx, KlFileOffset symbolpos) {
  KlSymTbl* symtbl = gen->symtbl;
  KlSymbol* symbol = klsymtbl_search(symtbl, name);
  if (kl_unlikely(symbol)) {
    klgen_error(gen, symbolpos, symbolpos,
                "redefinition of symbol: %*.s",
                symbol->name.length,
                klstrtab_getstring(gen->strtab, symbol->name.id));
    symbol->attr.idx = idx;
    return symbol;
  }
  symbol = klsymtbl_insert(symtbl, name);
  if (kl_unlikely(symbol))
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
  KlSymTbl* symtbl = klsymtblpool_alloc(gen->symtblpool, gen->strtab, gen->symtbl);
  klgen_oomifnull(gen, symtbl);
  gen->symtbl = symtbl;
  symtbl->info.stkbase = klgen_stacktop(gen);
}

void klgen_popsymtbl(KlGenUnit* gen) {
  KlSymTbl* symtbl = gen->symtbl;
  gen->symtbl = klsymtbl_parent(symtbl);
  klsymtblpool_dealloc(gen->symtblpool, symtbl);
}

void klgen_loadval(KlGenUnit* gen, size_t target, KlCodeVal val, KlFilePosition position) {
  switch (val.kind) {
    case KLVAL_STACK: {
      if (target != val.index)
        klgen_pushinst(gen, klinst_move(target, val.index), position);
      break;
    }
    case KLVAL_REF: {
      klgen_pushinst(gen, klinst_loadref(target, val.index), position);
      break;
    }
    case KLVAL_NIL: {
      KlInstruction inst = klinst_loadnil(target, 1);
      klgen_pushinst(gen, inst, position);
      break;
    }
    case KLVAL_BOOL: {
      KlInstruction inst = klinst_loadbool(target, val.boolval);
      klgen_pushinst(gen, inst, position);
      break;
    }
    case KLVAL_STRING: {
      KlConstant constant = { .type = KL_STRING, .string = val.string };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(gen, conent);
      klgen_pushinst(gen, klinst_loadc(target, conent->index), position);
      break;
    }
    case KLVAL_INTEGER: {
      KlInstruction inst;
      if (klinst_inrange(val.intval, 16)) {
        inst = klinst_loadi(target, val.intval);
      } else {
        KlConstant con = { .type = KL_INT, .intval = val.intval };
        KlConEntry* conent = klcontbl_get(gen->contbl, &con);
        klgen_oomifnull(gen, conent);
        inst = klinst_loadc(target, conent->index);
      }
      klgen_pushinst(gen, inst, position);
      break;
    }
    case KLVAL_FLOAT: {
      KlInstruction inst;
      KlConstant con = { .type = KL_FLOAT, .floatval = val.floatval };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(gen, conent);
      inst = klinst_loadc(target, conent->index);
      klgen_pushinst(gen, inst, position);
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
}

