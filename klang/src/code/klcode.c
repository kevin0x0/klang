#include "klang/include/code/klcode.h"
#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/code/klvalpos.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/parse/kllex.h"
#include "klang/include/parse/klstrtab.h"
#include "klang/include/vm/klinst.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

kgarray_impl(KlCode, KlCodeArray, klcodearr, pass_ref,)
kgarray_impl(KlInstruction, KlInstArray, klinstarr, pass_val,)

typedef struct tagKlFuncState KlFuncState;

struct tagKlFuncState {
  KlSymTbl* symtbl;           /* current symbol table */
  KlSymTbl* reftbl;           /* table that records references to upper klang function */
  KlConTbl* contbl;           /* constant table */
  KlSymTblPool* symtblpool;   /* object pool */
  KlCodeArray nestedfunc;     /* functions created inside this function */
  KlInstArray code;
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
  } config;
  struct {
    KlStrDesc constructor;
  } string;
};

static bool klfuncstate_init(KlFuncState* state, KlSymTblPool* symtblpool, KlStrTab* strtab, KlFuncState* prev, Ko* err, Ki* input);
static void klfuncstate_destroy(KlFuncState* state);

KlCode* klcode_create(KlCst* cst) {
}

void klcode_delete(KlCode* code) {
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

#define klcode_stacktop(state)            ((state)->stksize)
#define klcode_stackalloc(state)          ((state)->stksize++)
#define klcode_stackfree(state, stkid)    ((state)->stksize = (stkid))
#define klcode_stackfreeget(state, stkid) ((state)->stksize = (stkid + 1))

/* here begins the code generation */
typedef struct tagKlCodeVal {
  KlValKind kind;
  union {
    size_t index;
    KlInt intval;
    KlBool boolval;
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

static inline KlCodeVal klcodeval_constant(size_t index) {
  KlCodeVal val = { .kind = KLVAL_CONSTANT, .index = index };
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

static inline void klcode_pushinst(KlFuncState* state, KlInstruction inst) {
  if (kl_unlikely(!klinstarr_push_back(&state->code, inst)))
    klcode_error_fatal(state, "out of memory");
}



static KlSymbol* klcode_getsymbol(KlFuncState* state, KlStrDesc name) {
  if (state) return NULL;
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

static void klcode_putinstack(KlFuncState* state, KlCodeVal* val) {
  switch (val->kind) {
    case KLVAL_STACK: {
      break;
    }
    case KLVAL_REF: {
      size_t stkid = klcode_stackalloc(state);
      klcode_pushinst(state, klinst_loadref(stkid, val->index));
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_CONSTANT: {
      size_t stkid = klcode_stackalloc(state);
      klcode_pushinst(state, klinst_loadc(stkid, val->index));
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_NIL:
    case KLVAL_BOOL: {
      size_t stkid = klcode_stackalloc(state);
      KlInstruction inst;
      if (val->kind == KL_BOOL) {
        inst = klinst_loadbool(stkid, val->boolval);
      } else {
        kl_assert(val->kind == KL_NIL, "");
        inst = klinst_loadnil(stkid, 1);
      }
      klcode_pushinst(state, inst);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
    case KLVAL_INTEGER: {
      size_t stkid = klcode_stackalloc(state);
      KlInstruction inst;
      if (klinst_inrange(val->intval, 16)) {
        inst = klinst_loadi(stkid, val->intval);
      } else {
        KlConstant con = { .type = KL_INT, .intval = val->intval };
        KlConEntry* conent = klcontbl_get(state->contbl, &con);
        inst = klinst_loadc(stkid, conent->index);
      }
      klcode_pushinst(state, inst);
      val->kind = KLVAL_STACK;
      val->index = stkid;
      break;
    }
  }
}

static KlCodeVal klcode_expr(KlFuncState* state, KlCst* cst);
static inline KlCodeVal klcode_tuple_as_singleval(KlFuncState* state, KlCstTuple* tuplecst);
static inline KlCodeVal klcode_tuple(KlFuncState* state, KlCstTuple* tuplecst);
static inline KlCodeVal klcode_exprpost(KlFuncState* state, KlCstPost* postcst);
static inline KlCodeVal klcode_exprpre(KlFuncState* state, KlCstPre* precst);
static KlCodeVal klcode_constant(KlFuncState* state, KlCstConstant* concst);
static inline KlCodeVal klcode_identifier(KlFuncState* state, KlCstIdentifier* idcst);

static KlCodeVal klcode_constant(KlFuncState* state, KlCstConstant* concst) {
  switch (concst->con.type) {
    case KL_INT: {
      return klcodeval_integer(concst->con.intval);
    }
    case KL_BOOL: {
      return klcodeval_bool(concst->con.boolval);
    }
    case KL_STRING: {
      KlCodeVal val;
      KlConEntry* conent = klcontbl_get(state->contbl, &concst->con);
      klcode_oomifnull(conent);
      val.kind = KLVAL_CONSTANT;
      val.intval = conent->index;
      return val;
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
    size_t stkid = klcode_stackalloc(state);
    KlConstant con = { .type = KL_STRING, .string = idcst->id };
    KlConEntry* conent = klcontbl_get(state->contbl, &con);
    klcode_oomifnull(conent);
    klcode_pushinst(state, klinst_loadglobal(stkid, conent->index));
    return klcodeval_stack(stkid);
  }
}

static inline KlCodeVal klcode_tuple_as_singleval(KlFuncState* state, KlCstTuple* tuplecst) {
  size_t stkid = klcode_stacktop(state);
  if (tuplecst->nelem == 0) {
    klcode_pushinst(state, klinst_loadnil(stkid, 1));
    return klcodeval_stack(stkid);
  }
  KlCst** expr = tuplecst->elems;
  KlCst** end = expr + tuplecst->nelem;
  while (expr != end) {
    KlCodeVal res = klcode_expr(state, *expr++);
    klcode_stackfree(state, stkid);
    klcode_putinstack(state, &res);
    if (res.index != stkid) {
      klcode_pushinst(state, klinst_move(stkid, res.index));
    }
    klcode_stackfree(state, stkid);
  }
  return klcodeval_stack(stkid);
}

static inline KlCodeVal klcode_exprpre(KlFuncState* state, KlCstPre* precst) {
  switch (precst->op) {
    case KLTK_MINUS: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal val = klcode_expr(state, precst->operand);
      switch (val.kind) {
        case KLVAL_STACK: {
          klcode_pushinst(state, klinst_neg(stkid, val.index));
          return klcodeval_stack(stkid);
        }
        case KLVAL_REF: {
          klcode_pushinst(state, klinst_loadref(stkid, val.index));
          klcode_pushinst(state, klinst_neg(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_CONSTANT: {
          klcode_pushinst(state, klinst_loadc(stkid, val.index));
          klcode_pushinst(state, klinst_neg(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_BOOL: {
          klcode_pushinst(state, klinst_loadbool(stkid, val.boolval));
          klcode_pushinst(state, klinst_neg(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_NIL: {
          klcode_pushinst(state, klinst_loadnil(stkid, 1));
          klcode_pushinst(state, klinst_neg(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_INTEGER: {
          return klcodeval_integer(-val.intval);
        }
        default: {
          kl_assert(false, "control flow should not reach here");
          return klcodeval_nil();
        }
      }
    }
    case KLTK_ASYNC: {
      size_t stkid = klcode_stacktop(state);
      KlCodeVal val = klcode_expr(state, precst->operand);
      switch (val.kind) {
        case KLVAL_STACK: {
          klcode_pushinst(state, klinst_async(stkid, val.index));
          return klcodeval_stack(stkid);
        }
        case KLVAL_REF: {
          klcode_pushinst(state, klinst_loadref(stkid, val.index));
          klcode_pushinst(state, klinst_async(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_CONSTANT: {
          klcode_pushinst(state, klinst_loadc(stkid, val.index));
          klcode_pushinst(state, klinst_async(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_BOOL: {
          klcode_pushinst(state, klinst_loadbool(stkid, val.boolval));
          klcode_pushinst(state, klinst_async(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_NIL: {
          klcode_pushinst(state, klinst_loadnil(stkid, 1));
          klcode_pushinst(state, klinst_async(stkid, stkid));
          return klcodeval_stack(stkid);
        }
        case KLVAL_INTEGER: {
          klcode_stackfree(state, stkid);
          klcode_putinstack(state, &val);
          klcode_pushinst(state, klinst_async(stkid, val.index));
          return klcodeval_stack(stkid);
        }
        default: {
          kl_assert(false, "control flow should not reach here");
          return klcodeval_nil();
        }
      }
    }
    case KLTK_NOT: {
      /* TODO: implement boolean expression */
    }
    case KLTK_YIELD: {
      /* TODO: implement yield */
    }
  }
}

static KlCodeVal klcode_expr(KlFuncState* state, KlCst* cst) {
  KlCstBin* bin = klcast(KlCstBin*, cst);
  switch (klcst_kind(bin->roperand)) {
    case KLCST_EXPR_CONSTANT: {
      return klcode_constant(state, klcast(KlCstConstant*, cst));
    }
    case KLCST_EXPR_ID: {
      return klcode_identifier(state, klcast(KlCstIdentifier*, cst));
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
