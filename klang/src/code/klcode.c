#include "klang/include/code/klcode.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/parse/klstrtab.h"

kgarray_impl(KlCode, KlCodeArray, klcodearr,)

typedef struct tagKlFuncState KlFuncState;

struct tagKlFuncState {
  KlSymTbl* symtbl;           /* current symbol table */
  KlSymTbl* reftbl;           /* table that records references to upper klang function */
  KlConTbl* contbl;           /* constant table */
  KlSymTblPool* symtblpool;   /* object pool */
  KlCodeArray nestedfunc;     /* functions created inside this function */
  KlStrTab* strtab;
  size_t stksize;             /* current used stack size */
  size_t framesize;           /* stack frame size of this klang function */
  KlFuncState* prev;
};

static bool klfuncstate_init(KlFuncState* state, KlSymTblPool* symtblpool, KlStrTab* strtab, KlFuncState* prev);
static void klfuncstate_destroy(KlFuncState* state);



KlCode* klcode_create(KlCst* cst) {
}

void klcode_delete(KlCode* code) {
}

static bool klfuncstate_init(KlFuncState* state, KlSymTblPool* symtblpool, KlStrTab* strtab, KlFuncState* prev) {
  if (kl_unlikely(!(state->symtbl = klsymtblpool_alloc(symtblpool, strtab, NULL)))) {
    return false;
  }
  if (kl_unlikely(state->reftbl = klsymtblpool_alloc(symtblpool, strtab, NULL))) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    return false;
  }
  if (kl_unlikely(state->contbl = klcontbl_create(8, strtab))) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    klsymtblpool_dealloc(symtblpool, state->reftbl);
    return false;
  }
  state->symtblpool = symtblpool;
  if (kl_unlikely(!klcodearr_init(&state->nestedfunc, 2))) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    klsymtblpool_dealloc(symtblpool, state->reftbl);
    klcontbl_delete(state->contbl);
    return false;
  }
  state->strtab = strtab;
  state->stksize = 0;
  state->framesize = 0;
  state->prev = prev;
  return true;
}

static void klfuncstate_destroy(KlFuncState* state) {
  KlSymTbl* symtbl = state->symtbl;
  while (symtbl) {
    KlSymTbl* parent = klsymtbl_parent(symtbl);
    klsymtblpool_dealloc(state->symtblpool, symtbl);
    symtbl = parent;
  }
  klsymtblpool_dealloc(state->symtblpool, state->reftbl);
  klcontbl_delete(state->contbl);
  klcodearr_destroy(&state->nestedfunc);
}

