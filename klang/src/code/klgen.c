#include "klang/include/code/klgen.h"
#include "klang/include/code/klcode.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/vm/klinst.h"
#include <string.h>

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

bool klgen_init(KlGenUnit* gen, KlSymTblPool* symtblpool, KlStrTbl* strtbl, KlGenUnit* prev, Ki* input, KlError* klerror) {
  if (kl_unlikely(!(gen->symtbl = klsymtblpool_alloc(symtblpool, strtbl, NULL)))) {
    return false;
  }
  gen->reftbl = gen->symtbl;
  if (kl_unlikely(gen->contbl = klcontbl_create(8, strtbl))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    return false;
  }
  gen->symtblpool = symtblpool;
  if (kl_unlikely(!klcodearr_init(&gen->subfunc, 2))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    return false;
  }
  if (kl_unlikely(!klinstarr_init(&gen->code, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->subfunc);
    return false;
  }
  if (kl_unlikely(!klfparr_init(&gen->position, 32))) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->subfunc);
    klinstarr_destroy(&gen->code);
    return false;
  }
  gen->strtbl = strtbl;
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


  char* constructor = klstrtbl_newstring(strtbl, "constructor");
  char* itermethod = klstrtbl_newstring(strtbl, "<-");
  if (kl_unlikely(!constructor || !itermethod)) {
    klsymtblpool_dealloc(symtblpool, gen->symtbl);
    klsymtblpool_dealloc(symtblpool, gen->reftbl);
    klcontbl_delete(gen->contbl);
    klcodearr_destroy(&gen->subfunc);
    klinstarr_destroy(&gen->code);
    klfparr_destroy(&gen->position);
    return false;
  }
  gen->string.constructor.id = klstrtbl_stringid(gen->strtbl, constructor);
  gen->string.constructor.length = strlen("constructor");
  gen->string.itermethod.id = klstrtbl_stringid(gen->strtbl, itermethod);
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
  for (size_t i = 0; i < klcodearr_size(&gen->subfunc); ++i) {
    KlCode** code = klcodearr_access(&gen->subfunc, i);
    klcode_delete(*code);
  }
  klcodearr_destroy(&gen->subfunc);
  klinstarr_destroy(&gen->code);
}

KlCode* klgen_tocode_and_destroy(KlGenUnit* gen, size_t nparam) {
  klinstarr_shrink(&gen->code);
  klfparr_shrink(&gen->position);
  klcodearr_shrink(&gen->subfunc);
  KlConstant* constants = (KlConstant*)malloc(klcontbl_size(gen->contbl) * sizeof (KlConstant));
  KlRefInfo* refinfo = (KlRefInfo*)malloc(klsymtbl_size(gen->reftbl) * sizeof (KlRefInfo));
  if (kl_unlikely(!constants || !refinfo)) {
    free(constants);
    free(refinfo);
    klgen_error_fatal(gen, "out of memory");
  }
  klcontbl_setconstants(gen->contbl, constants);
  klreftbl_setrefinfo(gen->reftbl, refinfo);
  size_t codelen = klinstarr_size(&gen->code);
  KlInstruction* insts = klinstarr_steal(&gen->code);
  KlFilePosition* lineinfo = gen->config.debug ? klfparr_steal(&gen->position) : NULL;
  size_t nnested = klcodearr_size(&gen->subfunc);
  KlCode** nestedfunc = klcodearr_steal(&gen->subfunc);

  klgen_stackfree(gen, 0);
  KlCode* code = klcode_create(refinfo, klsymtbl_size(gen->reftbl), constants, klcontbl_size(gen->contbl),
                               insts, lineinfo, codelen, nestedfunc, nnested,
                               gen->strtbl, nparam, gen->framesize);
  kl_assert(gen->symtbl->parent == gen->reftbl, "");
  klsymtblpool_dealloc(gen->symtblpool, gen->symtbl);
  klsymtblpool_dealloc(gen->symtblpool, gen->reftbl);
  klcontbl_delete(gen->contbl);
  if (kl_unlikely(!code)) {
    for (size_t i = 0; i < nnested; ++i) {
      klcode_delete(nestedfunc[i]);
    }
    free(nestedfunc);
    free(insts);
    free(lineinfo);
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

KlSymbol* klgen_newsymbol(KlGenUnit* gen, KlStrDesc name, size_t idx, KlFilePosition symbolpos) {
  KlSymTbl* symtbl = gen->symtbl;
  KlSymbol* symbol = klsymtbl_search(symtbl, name);
  if (kl_unlikely(symbol)) {
    klgen_error(gen, symbolpos.begin, symbolpos.end,
                "redefinition of symbol: %*.s",
                symbol->name.length,
                klstrtbl_getstring(gen->strtbl, symbol->name.id));
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

void klgen_loadval(KlGenUnit* gen, size_t target, KlCodeVal val, KlFilePosition position) {
  switch (val.kind) {
    case KLVAL_STACK: {
      if (target != val.index)
        klgen_emitmove(gen, target, val.index, 1, position);
      break;
    }
    case KLVAL_REF: {
      klgen_emit(gen, klinst_loadref(target, val.index), position);
      break;
    }
    case KLVAL_NIL: {
      klgen_emitloadnils(gen, target, 1, position);
      break;
    }
    case KLVAL_BOOL: {
      KlInstruction inst = klinst_loadbool(target, val.boolval);
      klgen_emit(gen, inst, position);
      break;
    }
    case KLVAL_STRING: {
      KlConstant constant = { .type = KL_STRING, .string = val.string };
      KlConEntry* conent = klcontbl_get(gen->contbl, &constant);
      klgen_oomifnull(gen, conent);
      klgen_emit(gen, klinst_loadc(target, conent->index), position);
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
      klgen_emit(gen, inst, position);
      break;
    }
    case KLVAL_FLOAT: {
      KlInstruction inst;
      KlConstant con = { .type = KL_FLOAT, .floatval = val.floatval };
      KlConEntry* conent = klcontbl_get(gen->contbl, &con);
      klgen_oomifnull(gen, conent);
      inst = klinst_loadc(target, conent->index);
      klgen_emit(gen, inst, position);
      break;
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      break;
    }
  }
}

void klgen_emitloadnils(KlGenUnit* gen, size_t target, size_t nnil, KlFilePosition position) {
  if (klgen_currcodesize(gen) == 0) {
    klgen_emit(gen, klinst_loadnil(target, nnil), position);
  } else {
    KlInstruction* previnst = klinstarr_back(&gen->code);
    if (KLINST_GET_OPCODE(*previnst) == KLOPCODE_LOADNIL) {
      uint8_t prev_target = KLINST_AX_GETA(*previnst);
      uint8_t prev_nnil = KLINST_AX_GETX(*previnst);
      if ((prev_target <= target && target <= prev_target + prev_nnil) ||
          (target <= prev_target && prev_target <= target + nnil)) {
        uint8_t new_target = prev_target < target ? prev_target : target;
        uint8_t new_nnil = prev_nnil > nnil ? prev_nnil : nnil;
        *previnst = klinst_loadnil(new_target, new_nnil);
        return;
      } /* else fall through */
    }
    klgen_emit(gen, klinst_loadnil(target, nnil), position);
  }
}

void klgen_emitmove(KlGenUnit* gen, size_t target, size_t from, size_t nmove, KlFilePosition position) {
  if (nmove == 0) return;
  if (nmove == KLINST_VARRES) {
    klgen_emit(gen, klinst_multimove(target, from, nmove), position);
    return;
  }
  if (klgen_currcodesize(gen) == 0) {
    klgen_emit(gen, nmove == 1 ? klinst_move(target, from) : klinst_multimove(target, from, nmove), position);
  } else {
    KlInstruction* previnst = klinstarr_back(&gen->code);
    uint8_t prev_opcode = KLINST_GET_OPCODE(*previnst);
    if (prev_opcode == KLOPCODE_MOVE || prev_opcode == KLOPCODE_MULTIMOVE) {
      uint8_t prev_target = prev_opcode == KLOPCODE_MOVE ? KLINST_ABC_GETA(*previnst) : KLINST_ABX_GETA(*previnst);
      uint8_t prev_from = prev_opcode == KLOPCODE_MOVE ? KLINST_ABC_GETB(*previnst) : KLINST_ABX_GETB(*previnst);
      uint8_t prev_nmove = prev_opcode == KLOPCODE_MOVE ? 1 : KLINST_ABX_GETX(*previnst);
      if (prev_from + target == from + prev_target &&
          ((from <= prev_from && prev_from <= from + nmove) ||
          (prev_from <= from && from <= prev_from + prev_nmove))) {
        uint8_t new_target = prev_target < target ? prev_target : target;
        uint8_t new_from = prev_from < from ? prev_from : from;
        uint8_t new_nmove = prev_nmove > nmove ? prev_nmove : nmove;
        *previnst = new_nmove == 1 ? klinst_move(new_target, new_from) : klinst_multimove(new_target, new_from, new_nmove);
        return;
      } /* else fall through */
    }
    klgen_emit(gen, nmove == 1 ? klinst_move(target, from) : klinst_multimove(target, from, nmove), position);
  }
}
