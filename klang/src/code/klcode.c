#include "klang/include/code/klcode.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/code/klvalpos.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/vm/klinst.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

kgarray_impl(KlCode, KlCodeArray, klcodearr, pass_ref,)
kgarray_impl(KlInstruction, KlInstArray, klinstarr, pass_val,)
kgarray_impl(KlFilePosition, KlFPArray, klfparr, pass_val,)

typedef struct tagKlFuncState KlFuncState;

struct tagKlFuncState {
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
  KlFuncState* prev;
  Ko* err;
  Ki* input;
  size_t errcount;
  jmp_buf jmppos;
  struct {
    char* inputname;
    unsigned int tabstop;
    char curl;
    char zerocurl;
    bool debug;
  } config;
  struct {
    KlStrDesc constructor;
  } string;
};

static bool klfuncstate_init(KlFuncState* state, KlSymTblPool* symtblpool, KlStrTab* strtab, KlFuncState* prev, Ko* err, Ki* input);
static void klfuncstate_destroy(KlFuncState* state);

KlCode* klcode_create(KlCst* cst) {
  kltodo("implement klcode_create");
}

void klcode_delete(KlCode* code) {
  kltodo("implement klcode_delete");
}

static bool klfuncstate_init(KlFuncState* state, KlSymTblPool* symtblpool, KlStrTab* strtab, KlFuncState* prev, Ko* err, Ki* input) {
  if (kl_unlikely(!(state->symtbl = klsymtblpool_alloc(symtblpool, strtab, NULL)))) {
    return false;
  }
  state->reftbl = state->symtbl;
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
  if (kl_unlikely(!klinstarr_init(&state->code, 32))) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    klsymtblpool_dealloc(symtblpool, state->reftbl);
    klcontbl_delete(state->contbl);
    klcodearr_destroy(&state->nestedfunc);
    return false;
  }
  if (kl_unlikely(!klfparr_init(&state->position, 32))) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    klsymtblpool_dealloc(symtblpool, state->reftbl);
    klcontbl_delete(state->contbl);
    klcodearr_destroy(&state->nestedfunc);
    klinstarr_destroy(&state->code);
    return false;
  }
  state->strtab = strtab;
  state->stksize = 0;
  state->framesize = 0;
  state->prev = prev;

  state->err = err;
  state->input = input;
  state->errcount = 0;
  state->config.inputname = "unnamed";
  state->config.curl = '~';
  state->config.tabstop = 8;
  state->config.zerocurl = '^';

  int len = strlen("constructor");
  char* constructor = klstrtab_allocstring(strtab, len);
  if (kl_unlikely(!constructor)) {
    klsymtblpool_dealloc(symtblpool, state->symtbl);
    klsymtblpool_dealloc(symtblpool, state->reftbl);
    klcontbl_delete(state->contbl);
    klcodearr_destroy(&state->nestedfunc);
    return false;
  }
  strncpy(constructor, "constructor", len);
  state->string.constructor.id = klstrtab_pushstring(strtab, len);
  state->string.constructor.length = len;
  return true;
}

static void klfuncstate_destroy(KlFuncState* state) {
  KlSymTbl* symtbl = state->symtbl;
  while (symtbl) {
    KlSymTbl* parent = klsymtbl_parent(symtbl);
    klsymtblpool_dealloc(state->symtblpool, symtbl);
    symtbl = parent;
  }
  klcontbl_delete(state->contbl);
  klcodearr_destroy(&state->nestedfunc);
  klinstarr_destroy(&state->code);
}

static inline size_t klcode_stacktop(KlFuncState* state) {
  return state->stksize;
}

static inline size_t klcode_stackalloc(KlFuncState* state, size_t size) {
  size_t base = state->stksize;
  state->stksize += size;
  return base;
}

static inline size_t klcode_stackalloc1(KlFuncState* state) {
  return klcode_stackalloc(state, 1);
}

static inline void klcode_stackfree(KlFuncState* state, size_t stkid) {
  if (state->stksize > state->framesize)
    state->framesize = state->stksize;
  state->stksize = stkid;
}

static inline size_t klcode_currcodesize(KlFuncState* state) {
  return klinstarr_size(&state->code);
}

static inline void klcode_popinst(KlFuncState* state, size_t npop) {
  klinstarr_pop_back(&state->code, npop);
}

static inline void klcode_popinstto(KlFuncState* state, size_t to) {
  klcode_popinst(state, klcode_currcodesize(state) - to);
}

static inline KlFilePosition klcode_position(KlFileOffset begin, KlFileOffset end) {
  KlFilePosition position = { .begin = begin, .end = end };
  return position;
}

#define klcode_cstposition(cst) klcode_position(klcst_begin(cst), klcst_end(cst))

/* here begins the code generation */
typedef struct tagKlCodeVal {
  KlValKind kind;
  union {
    size_t index;
    KlInt intval;
    KlBool boolval;
    KlStrDesc string;
  };
} KlCodeVal;

static inline KlCodeVal klcodeval_index(KlValKind kind, size_t index) {
  KlCodeVal val = { .kind = kind, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_stack(size_t index) {
  KlCodeVal val = { .kind = KLVAL_STACK, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_ref(size_t index) {
  KlCodeVal val = { .kind = KLVAL_REF, .index = index };
  return val;
}

static inline KlCodeVal klcodeval_string(KlStrDesc str) {
  KlCodeVal val = { .kind = KLVAL_STRING, .string = str };
  return val;
}

static inline KlCodeVal klcodeval_integer(KlInt intval) {
  KlCodeVal val = { .kind = KLVAL_INTEGER, .intval = intval };
  return val;
}

static inline KlCodeVal klcodeval_bool(KlBool intval) {
  KlCodeVal val = { .kind = KLVAL_BOOL, .intval = intval };
  return val;
}

static inline KlCodeVal klcodeval_nil(void) {
  KlCodeVal val = { .kind = KLVAL_NIL };
  return val;
}



void klcode_error(KlFuncState* state, KlFileOffset begin, KlFileOffset end, const char* format, ...);

kl_noreturn static void klcode_error_fatal(KlFuncState* state, const char* message) {
  klcode_error(state, 0, 0, message);
  longjmp(state->jmppos, 1);
}

#define klcode_oomifnull(expr)  {                                               \
  if (kl_unlikely(!expr))                                                       \
    klcode_error_fatal(state, "out of memory");                                 \
}

static inline void klcode_pushinst(KlFuncState* state, KlInstruction inst, KlFilePosition position) {
  if (kl_unlikely(!klinstarr_push_back(&state->code, inst)))
    klcode_error_fatal(state, "out of memory");
  if (state->config.debug)
    klfparr_push_back(&state->position, position);
}

static inline void klcode_pushinstmethod(KlFuncState* state, size_t obj, size_t method, size_t narg, size_t nret, KlFilePosition position) {
  klcode_pushinst(state, klinst_method(obj, method), position);
  klcode_pushinst(state, klinst_methodextra(narg, nret), position);
}



static KlSymbol* klcode_getsymbol(KlFuncState* state, KlStrDesc name) {
  if (!state) return NULL;
  KlSymbol* symbol;
  KlSymTbl* symtbl = state->symtbl;
  while (symtbl) {
    symbol = klsymtbl_search(symtbl, name);
    if (symbol) break;
    symtbl = klsymtbl_parent(symtbl);
  }
  if (symbol) return symbol;
  KlSymbol* refsymbol = klcode_getsymbol(state->prev, name);
  if (!refsymbol) return NULL;  /* is global variable */
  symbol = klsymtbl_insert(state->reftbl, name);
  klcode_oomifnull(symbol);
  symbol->attr.kind = KLVAL_REF;
  symbol->attr.idx = klsymtbl_size(state->reftbl) - 1;
  symbol->attr.refto = refsymbol;
  return symbol;
}

static void klcode_putinstack(KlFuncState* state, KlCodeVal* val, KlFilePosition position) {
  switch (val->kind) {
    case KLVAL_STACK: {
      break;
    }
    case KLVAL_REF: {
      size_t stkid = klcode_stackalloc1(state);
      klcode_pushinst(state, klinst_loadref(stkid, val->index), position);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_STRING: {
      size_t stkid = klcode_stackalloc1(state);
      KlConstant constant = { .type = KL_STRING, .string = val->string };
      KlConEntry* conent = klcontbl_get(state->contbl, &constant);
      klcode_oomifnull(conent);
      klcode_pushinst(state, klinst_loadc(stkid, conent->index), position);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_NIL:
    case KLVAL_BOOL: {
      size_t stkid = klcode_stackalloc1(state);
      KlInstruction inst;
      if (val->kind == KL_BOOL) {
        inst = klinst_loadbool(stkid, val->boolval);
      } else {
        kl_assert(val->kind == KL_NIL, "");
        inst = klinst_loadnil(stkid, 1);
      }
      klcode_pushinst(state, inst, position);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_INTEGER: {
      size_t stkid = klcode_stackalloc1(state);
      KlInstruction inst;
      if (klinst_inrange(val->intval, 16)) {
        inst = klinst_loadi(stkid, val->intval);
      } else {
        KlConstant con = { .type = KL_INT, .intval = val->intval };
        KlConEntry* conent = klcontbl_get(state->contbl, &con);
        klcode_oomifnull(conent);
        inst = klinst_loadc(stkid, conent->index);
      }
      klcode_pushinst(state, inst, position);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
  }
}

static void klcode_putinstktop(KlFuncState* state, KlCodeVal* val, KlFilePosition position) {
  size_t stkid = klcode_stacktop(state);
  klcode_putinstack(state, val, position);
  if (val->index != stkid)
    klcode_pushinst(state, klinst_move(stkid, val->index), position);
  klcode_stackfree(state, stkid + 1);
}

static KlCodeVal klcode_expr(KlFuncState* state, KlCst* cst);
static inline KlCodeVal klcode_tuple_as_singleval(KlFuncState* state, KlCstTuple* tuplecst);
/* generate code that evaluates expressions in the tuple and put their values in the top of stack.
 * nwanted is the number of expected values. */
static void klcode_tuple(KlFuncState* state, KlCstTuple* tuplecst, size_t nwanted);
static size_t klcode_passargs(KlFuncState* state, KlCst* args);
static KlCodeVal klcode_exprpost(KlFuncState* state, KlCstPost* postcst);
static KlCodeVal klcode_exprpre(KlFuncState* state, KlCstPre* precst);
static KlCodeVal klcode_exprbin(KlFuncState* state, KlCstBin* bincst);
static KlCodeVal klcode_constant(KlFuncState* state, KlCstConstant* concst);
static inline KlCodeVal klcode_identifier(KlFuncState* state, KlCstIdentifier* idcst);

static KlCodeVal klcode_exprarr(KlFuncState* state, KlCstArray* arrcst) {
  size_t stkid = klcode_stacktop(state);
  size_t nval = klcode_passargs(state, arrcst->vals);
  klcode_pushinst(state, klinst_mkarray(stkid, stkid, nval), klcode_cstposition(arrcst));
  return klcodeval_stack(stkid);
}

static KlCodeVal klcode_exprarrgen(KlFuncState* state, KlCstArrayGenerator* arrgencst) {
  kltodo("implement arrgen");
}

inline static size_t abovelog2(size_t num) {
  size_t n = 0;
  while ((1 << n) < num)
    ++n;
  return n;
}

static KlCodeVal klcode_exprmap(KlFuncState* state, KlCstMap* mapcst) {
  size_t sizefield = abovelog2(mapcst->npair);
  if (sizefield < 3) sizefield = 3;
  size_t stkid = klcode_stacktop(state);
  klcode_pushinst(state, klinst_mkmap(stkid, stkid, sizefield), klcode_cstposition(mapcst));
  return klcodeval_stack(stkid);
}

static KlCodeVal klcode_exprclasspost(KlFuncState* state, KlCstClass* classcst) {
}

static KlCodeVal klcode_exprclass(KlFuncState* state, KlCstClass* classcst) {
  size_t stkid = klcode_stacktop(state);
  if (classcst->baseclass) {
    /* base is specified */
    KlCodeVal base = klcode_expr(state, classcst->baseclass);
    klcode_putinstack(state, &base, klcode_cstposition(classcst->baseclass));
    klcode_pushinst(state, klinst_mkclass(
  }
}

static KlCodeVal klcode_constant(KlFuncState* state, KlCstConstant* concst) {
  (void)state;
  switch (concst->con.type) {
    case KL_INT: {
      return klcodeval_integer(concst->con.intval);
    }
    case KL_BOOL: {
      return klcodeval_bool(concst->con.boolval);
    }
    case KL_STRING: {
      return klcodeval_string(concst->con.string);
    }
    case KL_NIL: {
      return klcodeval_nil();
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static inline KlCodeVal klcode_identifier(KlFuncState* state, KlCstIdentifier* idcst) {
  KlSymbol* symbol = klcode_getsymbol(state, idcst->id);
  if (symbol) {
    return klcodeval_index(symbol->attr.kind, symbol->attr.idx);
  } else {  /* else if global variable */
    size_t stkid = klcode_stackalloc1(state);
    KlConstant con = { .type = KL_STRING, .string = idcst->id };
    KlConEntry* conent = klcontbl_get(state->contbl, &con);
    klcode_oomifnull(conent);
    klcode_pushinst(state, klinst_loadglobal(stkid, conent->index), klcode_cstposition(idcst));
    return klcodeval_stack(stkid);
  }
}

static inline KlCodeVal klcode_tuple_as_singleval(KlFuncState* state, KlCstTuple* tuplecst) {
  if (tuplecst->nelem == 0)
    return klcodeval_nil();

  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem;
  KlCodeVal res;
  while (expr != end)
    res = klcode_expr(state, *expr++);
  return res;
}

static inline void klcode_expryield(KlFuncState* state, KlCstYield* yieldcst, size_t nwanted) {
  size_t stkid = klcode_stacktop(state);
  size_t nres = klcode_passargs(state, yieldcst->vals);
  klcode_pushinst(state, klinst_yield(stkid, nres, nwanted), klcode_cstposition(yieldcst));
  klcode_stackfree(state, stkid);
}

static void klcode_method(KlFuncState* state, KlCst* objcst, KlStrDesc method, KlCst* args, KlFilePosition position, size_t nret) {
  size_t stkid = klcode_stacktop(state);
  KlCodeVal obj = klcode_expr(state, objcst);
  klcode_putinstktop(state, &obj, klcode_cstposition(objcst));
  size_t narg = klcode_passargs(state, args);
  KlConstant con = { .type = KL_STRING, .string = method };
  KlConEntry* conent = klcontbl_get(state->contbl, &con);
  klcode_oomifnull(conent);
  klcode_pushinstmethod(state, stkid, conent->index, narg, nret, position);
  klcode_stackfree(state, stkid);
}

static void klcode_call(KlFuncState* state, KlCstPost* callcst, size_t nret) {
  if (klcst_kind(callcst->operand) == KLCST_EXPR_DOT) {
    KlCstDot* dotcst = klcast(KlCstDot*, callcst->operand);
    klcode_method(state, dotcst->operand, dotcst->field, callcst->post, klcode_cstposition(callcst), nret);
  }
  size_t stkid = klcode_stacktop(state);
  KlCodeVal callable = klcode_expr(state, callcst->operand);
  klcode_putinstktop(state, &callable, klcode_cstposition(callcst->operand));
  size_t narg = klcode_passargs(state, callcst->post);
  klcode_pushinst(state, klinst_call(callable.index, narg, nret), klcode_cstposition(callcst));
  klcode_stackfree(state, stkid);
}

static KlCodeVal klcode_multires(KlFuncState* state, KlCst* cst, size_t nres) {
  kl_assert(nres > 0, "");
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_YIELD: {
      size_t stkid = klcode_stacktop(state);
      klcode_expryield(state, klcast(KlCstYield*, cst), nres);
      return klcodeval_stack(stkid);
    }
    case KLCST_EXPR_CALL: {
      size_t stkid = klcode_stacktop(state);
      klcode_call(state, klcast(KlCstPost*, cst), nres);
      return klcodeval_stack(stkid);
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    default: {
      KlCodeVal res = klcode_expr(state, cst);
      klcode_putinstktop(state, &res, klcode_cstposition(cst));
      if (nres != 1) {
        klcode_pushinst(state, klinst_loadnil(res.index + 1, nres - 1), klcode_cstposition(cst));
        klcode_stackalloc(state, nres - 1);
      }
      return res;
    }
  }
}

static void klcode_tuple(KlFuncState* state, KlCstTuple* tuplecst, size_t nwanted) {
  size_t nvalid = nwanted < tuplecst->nelem ? nwanted : tuplecst->nelem;
  if (nvalid == 0) {
    if (nwanted == 0) return;
    size_t stktop = klcode_stacktop(state);
    klcode_pushinst(state, klinst_loadnil(stktop, nwanted), klcode_cstposition(tuplecst));
    klcode_stackalloc(state, nwanted);
    return;
  }
  KlCst** exprs = tuplecst->elems;
  size_t count = nvalid - 1;
  for (size_t i = 0; i < count; ++i) {
    KlCodeVal res = klcode_expr(state, exprs[i]);
    klcode_putinstktop(state, &res, klcode_cstposition(exprs[i]));
  }
  klcode_multires(state, exprs[count], nwanted - count);
}

static size_t klcode_passargs(KlFuncState* state, KlCst* args) {
  if (klcst_kind(args) == KLCST_EXPR_TUPLE) {
    size_t narg = klcast(KlCstTuple*, args)->nelem;
    klcode_tuple(state, klcast(KlCstTuple*, args), narg);
    return narg;
  } else {  /* else is a normal expression */
    KlCodeVal res = klcode_expr(state, args);
    klcode_putinstktop(state, &res, klcode_cstposition(args));
    return 1;
  }
}

static KlCodeVal klcode_exprpre(KlFuncState* state, KlCstPre* precst) {
  switch (precst->op) {
    case KLTK_MINUS: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal val = klcode_expr(state, precst->operand);
      if (val.kind == KLVAL_INTEGER)
        return klcodeval_integer(-val.intval);
      KlFilePosition pos = klcode_cstposition(precst);
      klcode_putinstack(state, &val, pos);
      klcode_stackfree(state, stkid);
      klcode_pushinst(state, klinst_neg(stkid, val.index), pos);
      return klcodeval_stack(stkid);
    }
    case KLTK_ASYNC: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal val = klcode_expr(state, precst->operand);
      KlFilePosition pos = klcode_cstposition(precst);
      klcode_putinstack(state, &val, pos);
      klcode_stackfree(state, stkid);
      klcode_pushinst(state, klinst_async(stkid, val.index), pos);
      return klcodeval_stack(stkid);
    }
    case KLTK_NOT: {
      kltodo("implement boolean expression");
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlInstruction klcode_bininst(KlCstBin* bincst, size_t stkid, size_t leftid, size_t rightid) {
  switch (bincst->op) {
    case KLTK_ADD:
      return klinst_add(stkid, leftid, rightid);
    case KLTK_MINUS:
      return klinst_sub(stkid, leftid, rightid);
    case KLTK_MUL:
      return klinst_mul(stkid, leftid, rightid);
    case KLTK_DIV:
      return klinst_div(stkid, leftid, rightid);
    case KLTK_MOD:
      return klinst_mod(stkid, leftid, rightid);
    case KLTK_CONCAT:
      return klinst_concat(stkid, leftid, rightid);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlInstruction klcode_bininsti(KlCstBin* bincst, size_t stkid, size_t leftid, KlInt imm) {
  switch (bincst->op) {
    case KLTK_ADD:
      return klinst_addi(stkid, leftid, imm);
    case KLTK_MINUS:
      return klinst_subi(stkid, leftid, imm);
    case KLTK_MUL:
      return klinst_muli(stkid, leftid, imm);
    case KLTK_DIV:
      return klinst_divi(stkid, leftid, imm);
    case KLTK_MOD:
      return klinst_modi(stkid, leftid, imm);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlInstruction klcode_bininstc(KlCstBin* bincst, size_t stkid, size_t leftid, size_t conidx) {
  switch (bincst->op) {
    case KLTK_ADD:
      return klinst_addc(stkid, leftid, conidx);
    case KLTK_MINUS:
      return klinst_subc(stkid, leftid, conidx);
    case KLTK_MUL:
      return klinst_mulc(stkid, leftid, conidx);
    case KLTK_DIV:
      return klinst_divc(stkid, leftid, conidx);
    case KLTK_MOD:
      return klinst_modc(stkid, leftid, conidx);
    default: {
      kl_assert(false, "control flow should not reach here");
      return 0;
    }
  }
}

static KlCodeVal klcode_exprbinleftliteral(KlFuncState* state, KlCstBin* bincst, KlCodeVal left) {
  kl_assert(left.kind == KLVAL_INTEGER || left.kind == KLVAL_STRING, "");
  /* left is not on the stack, so the stack top is not changed */
  size_t stkid = klcode_stacktop(state);
  size_t currcodesize = klcode_currcodesize(state);
  /* we put left on the stack first */
  KlCodeVal leftonstack = left;
  klcode_putinstack(state, &leftonstack, klcode_cstposition(bincst->loperand));
  KlCodeVal right = klcode_expr(state, bincst->roperand);
  if (right.kind != left.kind)
    klcode_putinstack(state, &right, klcode_cstposition(bincst->roperand));
  if (right.kind == KLVAL_STACK) {
    /* now we are sure that left should indeed be put on the stack */
    KlInstruction inst = klcode_bininst(bincst, stkid, leftonstack.index, right.index);
    klcode_pushinst(state, inst, klcode_cstposition(bincst));
    klcode_stackfree(state, stkid);
    return klcodeval_stack(stkid);
  } else if (left.kind == KLVAL_STRING) {
    /* right is string and now we know that left should not be put on the stack */
    kl_assert(klcode_currcodesize(state) - 1 == currcodesize, "");
    klcode_popinstto(state, currcodesize);  /* pop the instruction that put left on stack */
    char* res = klstrtab_concat(state->strtab, left.string, right.string);
    klcode_oomifnull(res);
    KlStrDesc str = { .id = klstrtab_stringid(state->strtab, res),
                      .length = left.string.length + right.string.length };
    return klcodeval_string(str);
  } else {
    /* right is integer and now we know that left should not be put on the stack */
    kl_assert(left.kind == KLVAL_INTEGER && right.kind == KLVAL_INTEGER, "");
    kl_assert(klcode_currcodesize(state) - 1 == currcodesize, "");
    klcode_popinstto(state, currcodesize);  /* pop the instruction that put left on stack */
    return klcodeval_integer(left.intval + right.intval);
  }
}

static KlCodeVal klcode_exprbinrightnonstk(KlFuncState* state, KlCstBin* bincst, size_t stkid, KlCodeVal left, KlCodeVal right) {
  /* left must be on stack */
  kl_assert(left.kind == KLVAL_STACK, "");
  switch (right.kind) {
    case KLVAL_INTEGER: {
      if (bincst->op == KLTK_CONCAT) {
        size_t rightid = klcode_stacktop(state);
        if (klinst_inrange(right.intval, 16)) {
          klcode_pushinst(state, klinst_loadi(rightid, right.intval), klcode_cstposition(bincst->roperand));
        } else {
          KlConstant con = { .type = KL_INT, .intval = right.intval };
          KlConEntry* conent = klcontbl_get(state->contbl, &con);
          klcode_oomifnull(conent);
          klcode_pushinst(state, klinst_loadc(rightid, conent->index), klcode_cstposition(bincst->roperand));
        }
        klcode_pushinst(state, klcode_bininst(bincst, stkid, left.index, rightid), klcode_cstposition(bincst));
      } else if (klinst_inrange(right.intval, 8)) {
        klcode_pushinst(state, klcode_bininsti(bincst, stkid, left.index, right.intval), klcode_cstposition(bincst));
      } else {
        KlConstant con = { .type = KL_INT, .intval = right.intval };
        KlConEntry* conent = klcontbl_get(state->contbl, &con);
        klcode_oomifnull(conent);
        if (klinst_inurange(conent->index, 8)) {
          klcode_pushinst(state, klcode_bininstc(bincst, stkid, left.index, conent->index), klcode_cstposition(bincst));
        } else {
          size_t stktop = klcode_stacktop(state);
          klcode_pushinst(state, klinst_loadc(stktop, conent->index), klcode_cstposition(bincst->roperand));
          klcode_pushinst(state, klcode_bininst(bincst, stkid, left.index, stktop), klcode_cstposition(bincst));
        }
      }
      klcode_stackfree(state, stkid);
      return klcodeval_stack(stkid);
    }
    case KLVAL_STRING: {
      KlConstant con = { .type = KL_STRING, .string = right.string };
      KlConEntry* conent = klcontbl_get(state->contbl, &con);
      klcode_oomifnull(conent);
      if (klinst_inurange(conent->index, 8) && bincst->op != KLTK_CONCAT) {
        klcode_pushinst(state, klcode_bininstc(bincst, stkid, left.index, conent->index), klcode_cstposition(bincst));
      } else {
        size_t stktop = klcode_stacktop(state);
        klcode_pushinst(state, klinst_loadc(stktop, conent->index), klcode_cstposition(bincst->roperand));
        klcode_pushinst(state, klcode_bininst(bincst, stkid, left.index, stktop), klcode_cstposition(bincst));
      }
      klcode_stackfree(state, stkid);
      return klcodeval_stack(stkid);
    }
    case KLVAL_REF:
    case KLVAL_BOOL:
    case KLVAL_NIL: {
      klcode_putinstack(state, &right, klcode_cstposition(bincst->roperand));
      klcode_pushinst(state, klcode_bininst(bincst, stkid, left.index, right.index), klcode_cstposition(bincst));
      klcode_stackfree(state, stkid);
      return klcodeval_stack(stkid);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klcode_exprbin(KlFuncState* state, KlCstBin* bincst) {
  if (kltoken_isarith(bincst->op) || bincst->op == KLTK_CONCAT) {
    size_t stkid = klcode_stacktop(state);
    KlCodeVal left = klcode_expr(state, bincst->loperand);
    if (left.kind == KLVAL_INTEGER || left.kind == KLVAL_STRING) {
      return klcode_exprbinleftliteral(state, bincst, left);
    } else if (left.kind != KLVAL_STACK) {  /* reference, constant, nil or bool */
      klcode_putinstack(state, &left, klcode_cstposition(bincst->loperand));
    } else if (left.index == klcode_stacktop(state)) {
      klcode_stackalloc1(state);
    }
    /* now left is on stack */
    KlCodeVal right = klcode_expr(state, bincst->roperand);
    if (left.kind != KLVAL_STACK) {
      return klcode_exprbinrightnonstk(state, bincst, stkid, left, right);
    } else if (left.index == klcode_stacktop(state)) {
      klcode_stackalloc1(state);
    }
    /* now both are on stack */
    klcode_stackfree(state, stkid);
    KlInstruction inst = klcode_bininst(bincst, stkid, left.index, right.index);
    klcode_pushinst(state, inst, klcode_cstposition(bincst));
    return klcodeval_stack(stkid);
  } else {  /* else is boolean expression */
    kltodo("implement boolean expression");
  }
}

static KlCodeVal klcode_exprpost(KlFuncState* state, KlCstPost* postcst) {
  switch (postcst->op) {
    case KLTK_CALL: {
      size_t stkid = klcode_stacktop(state);
      klcode_call(state, postcst, 1);
      return klcodeval_stack(stkid);
    }
    case KLTK_INDEX: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal indexable = klcode_expr(state, postcst->operand);
      klcode_putinstack(state, &indexable, klcode_cstposition(postcst->operand));
      KlCodeVal index = klcode_expr(state, postcst->post);
      if (index.kind == KLVAL_INTEGER && klinst_inrange(index.intval, 8)) {
        klcode_pushinst(state, klinst_indexi(stkid, indexable.index, index.intval), klcode_cstposition(postcst));
      } else {
        klcode_putinstack(state, &index, klcode_cstposition(postcst->post));
        klcode_pushinst(state, klinst_index(stkid, indexable.index, index.index), klcode_cstposition(postcst));
      }
      klcode_stackfree(state, stkid);
      return klcodeval_stack(stkid);
    }
    case KLTK_APPEND: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal appendable = klcode_expr(state, postcst->operand);
      klcode_putinstack(state, &appendable, klcode_cstposition(postcst->operand));
      size_t base = klcode_stacktop(state);
      size_t narg = klcode_passargs(state, postcst->post);
      klcode_pushinst(state, klinst_append(stkid, base, narg), klcode_cstposition(postcst));
      klcode_stackfree(state, stkid);
      return klcodeval_stack(appendable.index);
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}

static KlCodeVal klcode_exprnew(KlFuncState* state, KlCstNew* newcst) {
  size_t stkid = klcode_stacktop(state);
  KlCodeVal klclass = klcode_expr(state, newcst->klclass);
  klcode_putinstack(state, &klclass, klcode_cstposition(newcst->klclass));
  klcode_pushinst(state, klinst_newobj(stkid, klclass.index), klcode_cstposition(newcst));
  klcode_stackfree(state, stkid + 1);
  klcode_pushinst(state, klinst_move(stkid + 1, stkid), klcode_cstposition(newcst));
  klcode_stackfree(state, stkid + 2);
  size_t narg = klcode_passargs(state, newcst->args);
  KlConstant con = { .type = KL_STRING, .string = state->string.constructor };
  KlConEntry* conent = klcontbl_get(state->contbl, &con);
  klcode_oomifnull(conent);
  klcode_pushinstmethod(state, stkid + 1, conent->index, narg, 0, klcode_cstposition(newcst));
  klcode_stackfree(state, stkid);
  return klcodeval_stack(stkid);
}

static KlCodeVal klcode_exprdot(KlFuncState* state, KlCstDot* dotcst) {
  size_t stkid = klcode_stacktop(state);
  KlCodeVal obj = klcode_expr(state, dotcst->operand);
  KlConstant constant = { .type = KL_STRING, .string = dotcst->field };
  KlConEntry* conent = klcontbl_get(state->contbl, &constant);
  klcode_oomifnull(conent);
  if (obj.kind == KLVAL_REF) {
    if (klinst_inurange(conent->index, 8)) {
      klcode_pushinst(state, klinst_refgetfieldc(stkid, obj.index, conent->index), klcode_cstposition(dotcst));
    } else {
      klcode_pushinst(state, klinst_loadc(stkid, conent->index), klcode_cstposition(dotcst));
      klcode_pushinst(state, klinst_refgetfieldr(stkid, obj.index, stkid), klcode_cstposition(dotcst));
    }
    klcode_stackfree(state, stkid);
    return klcodeval_stack(stkid);
  } else {
    klcode_putinstack(state, &obj, klcode_cstposition(dotcst->operand));
    if (klinst_inurange(conent->index, 8)) {
      klcode_pushinst(state, klinst_getfieldc(stkid, obj.index, conent->index), klcode_cstposition(dotcst));
    } else {
      size_t stktop = klcode_stacktop(state);
      klcode_pushinst(state, klinst_loadc(stktop, conent->index), klcode_cstposition(dotcst));
      klcode_pushinst(state, klinst_getfieldr(stkid, obj.index, stktop), klcode_cstposition(dotcst));
    }
    klcode_stackfree(state, stkid);
    return klcodeval_stack(stkid);
  }
}

static KlCodeVal klcode_expr(KlFuncState* state, KlCst* cst) {
  switch (klcst_kind(cst)) {
    case KLCST_EXPR_ARR: {
      return klcode_exprarr(state, klcast(KlCstArray*, cst));
    }
    case KLCST_EXPR_ARRGEN: {
      return klcode_exprarrgen(state, klcast(KlCstArrayGenerator*, cst));
    }
    case KLCST_EXPR_MAP: {
      return klcode_exprmap(state, klcast(KlCstMap*, cst));
    }
    case KLCST_EXPR_CLASS: {
      kltodo("implement class");
    }
    case KLCST_EXPR_CONSTANT: {
      return klcode_constant(state, klcast(KlCstConstant*, cst));
    }
    case KLCST_EXPR_ID: {
      return klcode_identifier(state, klcast(KlCstIdentifier*, cst));
    }
    case KLCST_EXPR_VARARG: {
      kltodo("implement vararg");
    }
    case KLCST_EXPR_TUPLE: {
      return klcode_tuple_as_singleval(state, klcast(KlCstTuple*, cst));
    }
    case KLCST_EXPR_PRE: {
      return klcode_exprpre(state, klcast(KlCstPre*, cst));
    }
    case KLCST_EXPR_NEW: {
      return klcode_exprnew(state, klcast(KlCstNew*, cst));
    }
    case KLCST_EXPR_YIELD: {
      size_t stkid = klcode_stacktop(state);
      klcode_expryield(state, klcast(KlCstYield*, cst), 1);
      return klcodeval_stack(stkid);
    }
    case KLCST_EXPR_POST: {
      return klcode_exprpost(state, klcast(KlCstPost*, cst));
    }
    case KLCST_EXPR_DOT: {
      return klcode_exprdot(state, klcast(KlCstDot*, cst));
    }
    case KLCST_EXPR_FUNC: {
      kltodo("implement func");
    }
    case KLCST_EXPR_BIN: {
      return klcode_exprbin(state, klcast(KlCstBin*, cst));
    }
    case KLCST_EXPR_SEL: {
      kltodo("implement sel");
    }
    default: {
      kl_assert(false, "control flow should not reach here");
      return klcodeval_nil();
    }
  }
}



/* error handler */
size_t klcode_helper_locateline(Ki* input, size_t offset);
bool klcode_helper_showline_withcurl(KlFuncState* parser, Ki* input, KlFileOffset begin, KlFileOffset end);

void klcode_error(KlFuncState* state, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  ++state->errcount;
  Ko* err = state->err;
  Ki* input = state->input;
  size_t orioffset = ki_tell(input);
  size_t line = klcode_helper_locateline(input, begin);
  size_t linebegin = ki_tell(input);

  unsigned int col = begin - linebegin + 1;
  ko_printf(err, "%s:%4u:%4u: ", state->config.inputname, line, col);
  va_list vlst;
  va_start(vlst, format);
  ko_vprintf(err, format, vlst);
  va_end(vlst);
  ko_putc(err, '\n');

  while (klcode_helper_showline_withcurl(state, input, begin, end))
    continue;
  ki_seek(input, orioffset);
  ko_putc(err, '\n');
  ko_flush(err);
}

#define kl_isnl(ch)       ((ch) == '\n' || (ch) == '\r')

size_t klcode_helper_locateline(Ki* input, size_t offset) {
  ki_seek(input, 0);
  size_t currline = 1;
  size_t lineoff = 0;
  while (ki_tell(input) < offset) {
    int ch = ki_getc(input);
    if (ch == KOF) break;
    if (kl_isnl(ch)) {
      if ((ch = ki_getc(input)) != '\r' && ch != KOF)
        ki_ungetc(input);
      ++currline;
      lineoff = ki_tell(input);
    }
  }
  ki_seek(input, lineoff);
  return currline;
}

bool klcode_helper_showline_withcurl(KlFuncState* state, Ki* input, KlFileOffset begin, KlFileOffset end) {
  Ko* err = state->err;
  size_t curroffset = ki_tell(input);
  if (curroffset >= end) return false;
  int ch = ki_getc(input);
  while (!kl_isnl(ch) && ch != KOF) {
    ko_putc(err, ch);
    ch = ki_getc(input);
  }
  ko_putc(err, '\n');
  ki_seek(input, curroffset);
  ch = ki_getc(input);
  while (!kl_isnl(ch)) {
    if (curroffset == begin && curroffset == end) {
      ko_putc(err, state->config.zerocurl);
    } else if (curroffset >= begin && curroffset < end) {
      if (ch == '\t') {
        for (size_t i = 0; i < state->config.tabstop; ++i)
          ko_putc(err, state->config.curl);
      } else {
        ko_putc(err, state->config.curl);
      }
    } else {
      ko_putc(err, ch == '\t' ? '\t' : ' ');
    }
    if (ch == KOF) break;
    ch = ki_getc(input);
    ++curroffset;
  }
  ko_putc(err, '\n');
  if (ch != KOF && (ch = ki_getc(input)) != '\r' && ch != KOF)
    ki_ungetc(input);
  return true;
}
