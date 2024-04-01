#include "klang/include/code/klgen.h"
#include <string.h>

kgarray_impl(KlCode, KlCodeArray, klcodearr, pass_ref,)
kgarray_impl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_impl(KlFilePosition, KlFPArray, klfparr, pass_val,)


KlCode* klgen_create(KlCst* cst) {
  kltodo("implement klgen_create");
}

void klgen_delete(KlCode* code) {
  kltodo("implement klgen_delete");
}

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
  gen->jumpinfo = NULL;
  gen->prev = prev;
  gen->klerror = klerror;

  gen->input = input;
  gen->config.inputname = "unnamed";

  int len = strlen("constructor");
  char* constructor = klstrtab_allocstring(strtab, len);
  if (kl_unlikely(!constructor)) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->nestedfunc);
    return false;
  }
  strncpy(constructor, "constructor", len);
  gen->string.constructor.id = klstrtab_pushstring(strtab, len);
  gen->string.constructor.length = len;
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

KlSymbol* klgen_getsymbol(KlGenUnit* gen, KlStrDesc name) {
  if (!gen) return NULL;
  KlSymbol* symbol;
  KlSymTbl* symtbl = gen->symtbl;
  while (symtbl) {
    symbol = klsymtbl_search(symtbl, name);
    if (symbol) return symbol;
    symtbl = klsymtbl_parent(symtbl);
  }
  KlSymbol* refsymbol = klgen_getsymbol(gen->prev, name);
  if (!refsymbol) return NULL;  /* is global variable */
  symbol = klsymtbl_insert(gen->reftbl, name);
  klgen_oomifnull(symbol);
  symbol->attr.kind = KLVAL_REF;
  symbol->attr.idx = klsymtbl_size(gen->reftbl) - 1;
  symbol->attr.refto = refsymbol;
  return symbol;
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
      klgen_oomifnull(conent);
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
        klgen_oomifnull(conent);
        inst = klinst_loadc(target, conent->index);
      }
      klgen_pushinst(gen, inst, position);
      break;
    }
    case KLVAL_FLOAT: {
      KlInstruction inst;
      KlConstant con = { .type = KL_FLOAT, .floatval = val.floatval };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(conent);
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

