#include "include/parse/klparser.h"
#include "include/parse/klcfdarr.h"
#include "include/parse/klidarr.h"
#include "include/parse/kllex.h"
#include "include/parse/kltokens.h"
#include "include/error/klerror.h"
#include "include/cst/klcst.h"
#include "include/misc/klutils.h"
#include "deps/k/include/array/karray.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define KLCODE_GFOR_NSTATE  (2)


static KlFileOffset ph_filepos = ~(KlFileOffset)0;



bool klparser_init(KlParser* parser, KlStrTbl* strtbl, char* inputname, KlError* klerror) {
  parser->strtbl = strtbl;
  parser->inputname = inputname;
  parser->incid = 0;
  parser->klerror = klerror;

  int thislen = strlen("this");
  char* this = klstrtbl_allocstring(strtbl, thislen);
  if (kl_unlikely(!this)) return false;
  memcpy(this, "this", thislen * sizeof (char));
  parser->string.this.id = klstrtbl_pushstring(strtbl, thislen);
  parser->string.this.length = thislen;
  return true;
}

static KlStrDesc klparser_newtmpid(KlParser* parser, KlLex* lex) {
  char* newid = klstrtbl_allocstring(parser->strtbl, sizeof (size_t) * 8);
  if (kl_unlikely(!newid)) {
    klparser_error_oom(parser, lex);
    KlStrDesc str = { .id = 0, .length = 0 };
    return str;
  }
  newid[0] = '\0';  /* all temporary identifiers begin with '\0' */
  int len = sprintf(newid + 1, "%zu", parser->incid++) + 1;
  size_t strid = klstrtbl_pushstring(parser->strtbl, len);
  KlStrDesc str = { .id = strid, .length = len };
  return str;
}

/* recovery */
bool klparser_discarduntil(KlLex* lex, KlTokenKind kind) {
  while (lex->tok.kind != kind) {
    if (lex->tok.kind == KLTK_END) return false;
    kllex_next(lex);
  }
  return true;
}

bool klparser_discarduntil2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2) {
  while (lex->tok.kind != kind1 && lex->tok.kind != kind2) {
    if (lex->tok.kind == KLTK_END) return false;
    kllex_next(lex);
  }
  return true;
}

bool klparser_discardto(KlLex* lex, KlTokenKind kind) {
  bool success = klparser_discarduntil(lex, kind);
  if (success) kllex_next(lex);
  return success;
}

bool klparser_discardto2(KlLex* lex, KlTokenKind kind1, KlTokenKind kind2) {
  bool success = klparser_discarduntil2(lex, kind1, kind2);
  if (success) kllex_next(lex);
  return success;
}



#define klparser_karr_pushcst(arr, cst)  {              \
  if (kl_unlikely(!karray_push_back(arr, cst))) {       \
    klparser_error_oom(parser, lex);                    \
    if (cst) klcst_delete(cst);                         \
  }                                                     \
}

#define klparser_oomifnull(expr) {                      \
  if (kl_unlikely(!(expr)))                             \
    return klparser_error_oom(parser, lex);             \
}

#define klparser_returnifnull(expr) {                   \
  if (kl_unlikely(!(expr)))                             \
    return NULL;                                        \
}


KlCstStmtList* klparser_generator(KlParser* parser, KlLex* lex, KlCst* inner_stmt);
static KlCstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex);
static KlCstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex);

static KlCst* klparser_stmt_nosemi(KlParser* parser, KlLex* lex);


/* parser for expression */
static KlCstTuple* klparser_emptytuple(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end);
static KlCst* klparser_finishmap(KlParser* parser, KlLex* lex);
static KlCst* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlCst* expr, bool preparsed);
static KlCst* klparser_generatorclass(KlParser* parser, KlLex* lex);
static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static KlCst* klparser_array(KlParser* parser, KlLex* lex);
static KlCstTuple* klparser_finishtuple(KlParser* parser, KlLex* lex, KlCst* expr);
static KlCst* klparser_dotchain(KlParser* parser, KlLex* lex);
static KlCst* klparser_exprnew(KlParser* parser, KlLex* lex);

static inline bool klparser_exprbegin(KlLex* lex);

static inline bool klparser_exprbegin(KlLex* lex) {
  static bool isexprbegin[KLTK_NTOKEN] = {
    [KLTK_ARROW] = true,
    [KLTK_DARROW] = true,
    [KLTK_VARARG] = true,
    [KLTK_MINUS] = true,
    [KLTK_ADD] = true,
    [KLTK_NOT] = true,
    [KLTK_NEW] = true,
    [KLTK_INT] = true,
    [KLTK_STRING] = true,
    [KLTK_BOOLVAL] = true,
    [KLTK_NIL] = true,
    [KLTK_ID] = true,
    [KLTK_LBRACKET] = true,
    [KLTK_LBRACE] = true,
    [KLTK_YIELD] = true,
    [KLTK_ASYNC] = true,
    [KLTK_INHERIT] = true,
    [KLTK_METHOD] = true,
    [KLTK_FLOAT] = true,
    [KLTK_LPAREN] = true,
  };
  return isexprbegin[kllex_tokkind(lex)];
}

static inline KlCstTuple* klparser_exprlist(KlParser* parser, KlLex* lex) {
  KlCst* headexpr = klparser_expr(parser, lex);
  return headexpr == NULL ? NULL : klparser_finishtuple(parser, lex, headexpr);
}

static KlCstTuple* klparser_emptytuple(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlCstTuple* tuple = klcst_tuple_create(NULL, 0, begin, end);
  if (kl_unlikely(!tuple)) return klparser_error_oom(parser, lex);
  return tuple;
}

static KlCstTuple* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlCstTuple* exprlist = klcst_tuple_create(NULL, 0, begin, end);
  if (kl_unlikely(!exprlist)) return klparser_error_oom(parser, lex);
  return exprlist;
}

static inline KlCstTuple* klparser_exprlist_mayempty(KlParser* parser, KlLex* lex) {
  return klparser_exprbegin(lex) ? klparser_exprlist(parser, lex) : klparser_emptyexprlist(parser, lex, lex->tok.begin, lex->tok.begin);
}

static KlCstTuple* klparser_singletonexprlist(KlParser* parser, KlLex* lex, KlCst* expr) {
  KlCst** elems = (KlCst**)malloc(sizeof (KlCst*));
  if (kl_unlikely(!elems)) {
    klcst_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  elems[0] = expr;
  KlCstTuple* exprlist = klcst_tuple_create(elems, 1, expr->begin, expr->end);
  if (kl_unlikely(!exprlist))
    return klparser_error_oom(parser, lex);
  return exprlist;
}

/* tool for clean cst karray when error occurred. */
static void klparser_destroy_cstarray(KArray* arr) {
  for (size_t i = 0; i < karray_size(arr); ++i) {
    KlCst* cst = (KlCst*)karray_access(arr, i);
    /* cst may be NULL when error occurred */
    if (cst) klcst_delete(cst);
  }
  karray_destroy(arr);
}

KlCst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis) {
  if (inparenthesis) *inparenthesis = false;
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlCstConstant* cst = klcst_constant_create_integer(lex->tok.intval, lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_FLOAT: {
      KlCstConstant* cst = klcst_constant_create_float(lex->tok.floatval, lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_STRING: {
      KlCstConstant* cst = klcst_constant_create_string(lex->tok.string, lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_BOOLVAL: {
      KlCstConstant* cst = klcst_constant_create_boolean(lex->tok.boolval, lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_NIL: {
      KlCstConstant* cst = klcst_constant_create_nil(lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_ID: {
      KlCstIdentifier* cst = klcst_id_create(lex->tok.string, lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_VARARG: {
      KlCstVararg* cst = klcst_vararg_create(lex->tok.begin, lex->tok.end);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klcst(cst);
    }
    case KLTK_LBRACKET: {
      return klcst(klparser_array(parser, lex));
    }
    case KLTK_LBRACE: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* cst = kllex_check(lex, KLTK_COLON) ? klparser_finishmap(parser, lex) : klparser_generatorclass(parser, lex);
      KlFileOffset end = lex->tok.end;
      klparser_match(parser, lex, KLTK_RBRACE);
      if (kl_likely(cst)) klcst_setposition(cst, begin, end);
      return cst;
    }
    case KLTK_LPAREN: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlFileOffset mayend = lex->tok.end;
      if (kllex_trymatch(lex, KLTK_RPAREN)) { /* empty tuple */
        KlCstTuple* tuple = klparser_emptytuple(parser, lex, begin, mayend);
        klparser_returnifnull(tuple);
        return klcst(tuple);
      }
      /* may be a tuple */
      KlCst* expr = klparser_expr(parser, lex);
      if (kllex_check(lex, KLTK_RPAREN)) {
        if (inparenthesis) *inparenthesis = true;
        klcst_setposition(expr, begin, lex->tok.end);
        kllex_next(lex);
        return expr;
      }
      /* it is a tuple */
      klparser_returnifnull(expr);
      KlCstTuple* tuple = klparser_finishtuple(parser, lex, expr);
      KlFileOffset end = lex->tok.end;
      klparser_match(parser, lex, KLTK_RPAREN);
      klparser_returnifnull(tuple);
      klcst_setposition(tuple, begin, end);
      return klcst(tuple);
    }
    default: {
      klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end,
                     "expect '(', '{', '[', true, false, identifier, string or integer");
      return NULL;
    }
  }
}

static KlCst* klparser_array(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACKET), "expect \'[\'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);  /* skip '[' */
  KlCstTuple* exprlist = klparser_exprlist_mayempty(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discardto(lex, KLTK_RBRACKET);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_BAR)) { /* array generator */
    KlCstIdentifier* arrid = klcst_id_create(klparser_newtmpid(parser, lex), klcst_begin(exprlist), klcst_end(exprlist));
    klparser_oomifnull(arrid);
    KlCstPost* exprpush = klcst_post_create(KLTK_APPEND, klcst(arrid), klcst(exprlist), klcst_begin(exprlist), klcst_end(exprlist));
    klparser_oomifnull(exprpush);
    KlCstTuple* tuple = klparser_singletonexprlist(parser, lex, klcst(exprpush));
    klparser_oomifnull(tuple);
    KlCstStmtExpr* inner_stmt = klcst_stmtexpr_create(tuple, klcst_begin(exprlist), klcst_end(exprlist));
    klparser_oomifnull(inner_stmt);
    KlCstStmtList* stmtlist = klparser_generator(parser, lex, klcast(KlCst*, inner_stmt));
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RBRACKET);
    klparser_returnifnull(stmtlist);  /* 'inner_stmt' will be deleted in klparser_generator() when error occurred */
    KlCstArrayGenerator* array = klcst_arraygenerator_create(arrid->id, stmtlist, begin, end);
    return klcst(array);
  } else {
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RBRACKET);
    KlCstArray* array = klcst_array_create(exprlist, begin, end);
    klparser_oomifnull(array);
    return klcst(array);
  }
}

static KlCstTuple* klparser_finishtuple(KlParser* parser, KlLex* lex, KlCst* expr) {
  KArray exprs;
  if (kl_unlikely(!karray_init(&exprs)))
    return klparser_error_oom(parser, lex);
  if (kl_unlikely(!karray_push_back(&exprs, expr))) {
    klparser_destroy_cstarray(&exprs);
    klcst_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  while (kllex_trymatch(lex, KLTK_COMMA) && klparser_exprbegin(lex)) {
    KlCst* expr = klparser_expr(parser, lex);
    if (kl_unlikely(!expr)) continue;
    klparser_karr_pushcst(&exprs, expr);
  }
  karray_shrink(&exprs);
  size_t nelem = karray_size(&exprs);
  KlCstTuple* tuple = klcst_tuple_create((KlCst**)karray_steal(&exprs), nelem, expr->begin, klcst_end(karray_top(&exprs)));
  klparser_oomifnull(tuple);
  return tuple;
}

static KlCst* klparser_finishmap(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_COLON), "expect ':'");
  kllex_next(lex);
  KArray keys;
  KArray vals;
  if (kl_unlikely(!karray_init(&keys) || !karray_init(&vals))) {
    karray_destroy(&keys);
    karray_destroy(&vals);
    return klparser_error_oom(parser, lex);
  }
  if (!kllex_check(lex, KLTK_COLON)) {  /* non-empty */
    /* parse all key-value pairs */
    do {
      KlCst* key = klparser_expr(parser, lex);
      klparser_match(parser, lex, KLTK_COLON);
      KlCst* val = klparser_expr(parser, lex);
      if (kl_likely(key && val)) {
        klparser_karr_pushcst(&keys, key);
        klparser_karr_pushcst(&vals, val);
      } else {
        if (key) klcst_delete(key);
        if (val) klcst_delete(val);
      }
    } while (kllex_trymatch(lex, KLTK_COMMA));
  }
  klparser_match(parser, lex, KLTK_COLON);
  if (kl_unlikely(karray_size(&keys) != karray_size(&vals))) {
    klparser_destroy_cstarray(&keys);
    klparser_destroy_cstarray(&vals);
    return NULL;
  }
  karray_shrink(&keys);
  karray_shrink(&vals);
  size_t npair = karray_size(&keys);
  KlCstMap* map = klcst_map_create((KlCst**)karray_steal(&keys), (KlCst**)karray_steal(&vals), npair, ph_filepos, ph_filepos);
  klparser_oomifnull(map);
  return klcst(map);
}

static KlCst* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlCst* expr, bool preparsed) {
  KlCfdArray fields;
  KArray vals;
  if (kl_unlikely(!klcfd_init(&fields, 16) || !karray_init(&vals))) {
    klcfd_destroy(&fields);
    karray_destroy(&vals);
    return klparser_error_oom(parser, lex);
  }
  if (preparsed) {
    /* insert first shared k-v pair */
    KlCstClassFieldDesc cfd;
    cfd.shared = true;
    cfd.name = id;
    if (kl_unlikely(!klcfd_push_back(&fields, &cfd)))
      klparser_error_oom(parser, lex);
    if (kl_unlikely(!karray_push_back(&vals, expr))) {
      if (expr) klcst_delete(expr);
      klparser_error_oom(parser, lex);
    }
    if (kllex_trymatch(lex, KLTK_COMMA))  /* has trailing shared k-v pairs */
      klparser_sharedlist(parser, lex, &fields, &vals);
    kllex_trymatch(lex, KLTK_SEMI);
  }
  while (!kllex_check(lex, KLTK_RBRACE)) {
    if (kllex_check(lex, KLTK_LOCAL)) {
      kllex_next(lex);
      klparser_locallist(parser, lex, &fields, &vals);
      kllex_trymatch(lex, KLTK_SEMI);
    } else if (kl_likely(!kllex_check(lex, KLTK_END))) {
      kllex_trymatch(lex, KLTK_SHARED);
      klparser_sharedlist(parser, lex, &fields, &vals);
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "end of file");
      break;
    }
  }
  if (kl_unlikely(klcfd_size(&fields) != karray_size(&vals))) {
    klcfd_destroy(&fields);
    klparser_destroy_cstarray(&vals);
    return NULL;
  }
  klcfd_shrink(&fields);
  karray_shrink(&vals);
  size_t nfield = karray_size(&vals);
  KlCstClass* klclass = klcst_class_create(klcfd_steal(&fields), (KlCst**)karray_steal(&vals), nfield, NULL, ph_filepos, ph_filepos);
  klparser_oomifnull(klclass);
  return klcst(klclass);
}

static KlCst* klparser_generatorclass(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_RBRACE) ||
      kllex_check(lex, KLTK_LOCAL)  ||
      kllex_check(lex, KLTK_SHARED)) {  /* is class */
    KlStrDesc id = { 0, 0 };  /* the value is unimportant here */
    return klparser_finishclass(parser, lex, id, NULL, false);
  }

  KlCstTuple* exprlist = klcast(KlCstTuple*, klparser_exprlist(parser, lex));
  if (kl_unlikely(!exprlist)) {
    klparser_discarduntil(lex, KLTK_RBRACE);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* a class? */
    if (kl_unlikely(exprlist->nelem != 1                            ||
                    klcst_kind(exprlist->elems[0]) != KLCST_EXPR_ID)) {
      klparser_error(parser, kllex_inputstream(lex),
                     klcst_begin(exprlist), klcst_end(exprlist),
                     "should be a single identifier in class definition");
      klcst_delete(klcast(KlCst*, exprlist));
      klparser_discarduntil(lex, KLTK_RBRACE);
      return NULL;
    }
    KlStrDesc id = klcast(KlCstIdentifier*, exprlist->elems[0])->id;
    klcst_delete(klcast(KlCst*, exprlist));
    KlCst* expr = klparser_expr(parser, lex);
    return klparser_finishclass(parser, lex, id, expr, true);
  }
  /* else is a generator */
  klparser_match(parser, lex, KLTK_BAR);
  KlCstYield* yieldexpr = klcst_yield_create(exprlist, klcst_begin(exprlist), klcst_end(exprlist));
  klparser_oomifnull(yieldexpr);
  KlCstTuple* tuple = klparser_singletonexprlist(parser, lex, klcst(yieldexpr));
  klparser_oomifnull(tuple);
  KlCstStmtExpr* stmtexpr = klcst_stmtexpr_create(tuple, klcst_begin(yieldexpr),  klcst_end(yieldexpr));
  klparser_oomifnull(stmtexpr);
  KlCstStmtList* generator = klparser_generator(parser, lex, klcst(stmtexpr));
  klparser_returnifnull(generator);
  KlCstTuple* params = klparser_emptyexprlist(parser, lex, klcst_begin(stmtexpr), klcst_begin(stmtexpr));
  if (kl_unlikely(!params)) {
    klcst_delete(generator);
    return NULL;
  }
  KlCstFunc* func = klcst_func_create(generator, params, false, false, klcst_begin(stmtexpr), klcst_end(generator));
  klparser_oomifnull(func);
  KlCstPre* asyncexpr = klcst_pre_create(KLTK_ASYNC, klcst(func), ph_filepos, ph_filepos);
  klparser_oomifnull(asyncexpr);
  return klcst(asyncexpr);
}

static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc) || !karray_push_back(vals, NULL))) {
      klparser_error_oom(parser, lex);
    }
    kllex_next(lex);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return;
}

static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    kllex_next(lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlCst* val = klparser_expr(parser, lex);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc)))
      klparser_error_oom(parser, lex);
    klparser_karr_pushcst(vals, val);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return;
}

static KlCst* klparser_dotchain(KlParser* parser, KlLex* lex) {
  KlCst* dotexpr = klparser_exprunit(parser, lex, NULL);
  while (kllex_trymatch(lex, KLTK_DOT)) {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      continue;
    if (kl_unlikely(!dotexpr)) {
      kllex_next(lex);
      continue;
    }
    KlCstDot* dot = klcst_dot_create(dotexpr, lex->tok.string, dotexpr->begin, lex->tok.end);
    klparser_oomifnull(dot);
    dotexpr = klcst(dot);
    kllex_next(lex);
  }
  return dotexpr;
}

static KlCst* klparser_exprnew(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_NEW), "");

  kllex_next(lex);
  KlCst* klclass = kllex_check(lex, KLTK_LPAREN) ? klparser_exprunit(parser, lex, NULL) : klparser_dotchain(parser, lex);
  kltodo("must have (patameters)");
  KlCst* args = klparser_exprunit(parser, lex, NULL);
  if (kl_unlikely(!klclass || !args)) {
    if (klclass) klcst_delete(klclass);
    if (args) klcst_delete(args);
    return NULL;
  }
  KlCstNew* newexpr = klcst_new_create(klclass, klcast(KlCstTuple*, args), klclass->begin, args->end);
  klparser_oomifnull(newexpr);
  return klcst(newexpr);
}

KlCst* klparser_exprpre(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlCstPre* neg = klcst_pre_create(KLTK_MINUS, expr, begin, expr->end);
      klparser_oomifnull(neg);
      return klcst(neg);
    }
    case KLTK_NOT: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlCstPre* notexpr = klcst_pre_create(KLTK_NOT, expr, begin, expr->end);
      klparser_oomifnull(notexpr);
      return klcst(notexpr);
    }
    case KLTK_ARROW: {
      KlFileOffset begin = lex->tok.begin;
      KlCstStmtList* block = klparser_arrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlCstTuple* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klcst_delete(block);
        return NULL;
      }
      KlCstFunc* func = klcst_func_create(block, params, false, false, begin, klcst_end(block));
      klparser_oomifnull(func);
      return klcst(func);
    }
    case KLTK_DARROW: {
      KlFileOffset begin = lex->tok.begin;
      KlCstStmtList* block = klparser_darrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlCstTuple* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klcst_delete(block);
        return NULL;
      }
      KlCstFunc* func = klcst_func_create(block, params, false, false, begin, klcst_end(block));
      klparser_oomifnull(func);
      return klcst(func);
    }
    case KLTK_YIELD: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCstTuple* exprlist = klparser_exprlist_mayempty(parser, lex);
      klparser_returnifnull(exprlist);
      KlCstYield* yieldexpr = klcst_yield_create(exprlist, begin, klcst_end(exprlist));
      klparser_oomifnull(yieldexpr);
      return klcst(yieldexpr);
    }
    case KLTK_ASYNC: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlCstPre* asyncexpr = klcst_pre_create(KLTK_ASYNC, expr, begin, expr->end);
      klparser_oomifnull(asyncexpr);
      return klcst(asyncexpr);
    }
    case KLTK_INHERIT: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* base = klparser_exprpre(parser, lex);
      klparser_returnifnull(base);
      klparser_match(parser, lex, KLTK_COLON);
      KlCst* cst = klparser_exprunit(parser, lex, NULL);
      if (kl_unlikely(!cst)) {
        klcst_delete(base);
        return NULL;
      }
      if (kl_unlikely(klcst_kind(cst) != KLCST_EXPR_CLASS)) {
        klparser_error(parser, kllex_inputstream(lex), cst->begin, cst->end,
                       "must be a class definition");
        klcst_delete(base);
        klcst_delete(cst);
        return NULL;
      }
      KlCstClass* klclass = klcast(KlCstClass*, cst);
      klclass->baseclass = base;
      klcst_setposition(klclass, begin, klcst_end(klclass));
      return klcst(klclass);
    }
    case KLTK_METHOD: {
      KlFileOffset begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      if (kl_unlikely(klcst_kind(expr) != KLCST_EXPR_FUNC)) {
        klparser_error(parser, kllex_inputstream(lex), expr->begin, expr->end,
                       "'method' must be followed by a function construction");
        return expr;
      }
      KlCst* idthis = klcst(klcst_id_create(parser->string.this, klcst_begin(expr), klcst_begin(expr)));
      if (kl_unlikely(!idthis)) {
        klcst_delete(expr);
        return klparser_error_oom(parser, lex);
      }
      KlCstFunc* func = klcast(KlCstFunc*, expr);
      KlCstTuple* funcparams = klcast(KlCstTuple*, func->params);
      KlCst** newarr = (KlCst**)realloc(funcparams->elems, (funcparams->nelem + 1) * sizeof (KlCst*));
      if (kl_unlikely(!newarr)) {
        klcst_delete(expr);
        klcst_delete(idthis);
        return klparser_error_oom(parser, lex);
      }
      memmove(newarr + 1, newarr, funcparams->nelem * sizeof (KlCst*));
      newarr[0] = idthis;   /* add a parameter named 'this' */
      funcparams->elems = newarr;
      ++funcparams->nelem;
      func->is_method = true;
      klcst_setposition(func, begin, expr->end);
      return klcst(func);
    }
    case KLTK_ADD: {
      kllex_next(lex);
      return klparser_exprpre(parser, lex);
    }
    case KLTK_NEW: {
      return klparser_exprnew(parser, lex);
    }
    default: {  /* no prefix */
      return klparser_exprpost(parser, lex);
    }
  }
}

static KlCstTuple* klparser_correctfuncparams(KlParser* parser, KlLex* lex, KlCst* expr, bool* vararg) {
  *vararg = false;
  if (klcst_kind(expr) == KLCST_EXPR_TUPLE) {
    KlCstTuple* params = klcast(KlCstTuple*, expr);
    KlCst** exprs = params->elems;
    size_t nexpr = params->nelem;
    for (size_t i = 0; i < nexpr; ++i) {
      if (klcst_kind(exprs[i]) == KLCST_EXPR_VARARG) {
        if (kl_unlikely(i != nexpr - 1))
          klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "'...' can only appear at the end of the parameter list");
        *vararg = true;
        continue;
      }
    }
    if (*vararg) {
      klcst_delete(params->elems[params->nelem - 1]);
      --params->nelem;
    }
    return params;
  } else if (klcst_kind(expr) == KLCST_EXPR_VARARG) {
    *vararg = true;
    size_t begin = klcst_begin(expr);
    size_t end = klcst_end(expr);
    klcst_delete(expr);
    return klparser_emptyexprlist(parser, lex, begin, end);
  } else {
    return klparser_singletonexprlist(parser, lex, expr);
  }
}

static KlCstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_DARROW), "");
  kllex_next(lex);
  if (kllex_check(lex, KLTK_LPAREN)) {
    KlFileOffset begin = lex->tok.begin;
    kllex_next(lex);
    KlCstTuple* exprlist = klparser_exprlist(parser, lex);
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RPAREN);
    klparser_returnifnull(exprlist);
    KlCstStmtReturn* stmtreturn = klcst_stmtreturn_create(exprlist, begin, end);
    klparser_oomifnull(stmtreturn);
    KlCst** stmts = (KlCst**)malloc(sizeof (KlCst**));
    if (kl_unlikely(!stmts)) {
      klcst_delete(stmtreturn);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = klcst(stmtreturn);
    KlCstStmtList* block = klcst_stmtlist_create(stmts, 1, klcst_begin(stmtreturn), klcst_end(stmtreturn));
    klparser_oomifnull(block);
    return block;
  } else {
    klparser_check(parser, lex, KLTK_LBRACE);
    KlFileOffset begin = lex->tok.begin;
    kllex_next(lex);
    KlCstStmtList* block = klparser_stmtlist(parser, lex);
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RBRACE);
    klparser_returnifnull(block);
    klcst_setposition(block, begin, end);
    return block;
  }
}

static KlCstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_ARROW), "");
  kllex_next(lex);
  KlCst* expr = klparser_expr(parser, lex);
  klparser_returnifnull(expr);
  KlCstTuple* retvals = klparser_singletonexprlist(parser, lex, expr);
  klparser_returnifnull(retvals);
  KlCstStmtReturn* stmtreturn = klcst_stmtreturn_create(retvals, expr->begin, expr->end);
  klparser_oomifnull(stmtreturn);
  KlCst** stmts = (KlCst**)malloc(sizeof (KlCst**));
  if (kl_unlikely(!stmts)) {
    klcst_delete(stmtreturn);
    return klparser_error_oom(parser, lex);
  }
  stmts[0] = klcst(stmtreturn);
  KlCstStmtList* block = klcst_stmtlist_create(stmts, 1, klcst_begin(stmtreturn), klcst_end(stmtreturn));
  klparser_oomifnull(block);
  return block;
}

static KlCstCall* klparser_exprcall(KlParser* parser, KlLex* lex, KlCst* callable) {
  bool is_singleton = kllex_hasleadingblank(lex);
  if (is_singleton) {
    KlCst* unit = klparser_exprunit(parser, lex, NULL);
    if (kl_unlikely(!unit)) {
      if (callable) klcst_delete(callable);
      return NULL;
    }
    KlCstTuple* params = klparser_singletonexprlist(parser, lex, unit);
    if (kl_unlikely(!params)) {
      if (callable) klcst_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klcst_delete(params);
      return NULL;
    }
    KlCstCall* call = klcst_call_create(callable, params, callable->begin, klcst_end(unit));
    klparser_oomifnull(call);
    return call;
  } else {
    KlFileOffset begin = lex->tok.begin;
    klparser_match(parser, lex, KLTK_LPAREN);
    KlCstTuple* params = klparser_exprlist_mayempty(parser, lex);
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RPAREN);
    if (kl_unlikely(!params)) {
      if (callable) klcst_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klcst_delete(params);
      return NULL;
    }
    klcst_setposition(params, begin, end);
    KlCstCall* call = klcst_call_create(callable, params, callable->begin, klcst_end(params));
    klparser_oomifnull(call);
    return call;
  }
}

KlCst* klparser_exprwhere(KlParser* parser, KlLex* lex, KlCst* expr) {
  kllex_next(lex);
  KlCstStmtList* block;
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFileOffset begin = lex->tok.begin;
    kllex_next(lex);
    block = klparser_stmtlist(parser, lex);
    KlFileOffset end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RBRACE);
    if (kl_unlikely(!block)) {
      klcst_delete(expr);
      return NULL;
    }
    klcst_setposition(block, begin, end);
  } else {
    KlCst* stmt = klparser_stmt_nosemi(parser, lex);
    if (kl_unlikely(!stmt)) {
      klcst_delete(expr);
      return NULL;
    }
    KlCst** stmts = (KlCst**)malloc(1 * sizeof (KlCst*));
    if (kl_unlikely(!stmts)) {
      klcst_delete(stmt);
      klcst_delete(expr);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    block = klcst_stmtlist_create(stmts, 1, klcst_begin(stmt), klcst_end(stmt));
    if (kl_unlikely(!block)) {
      klcst_delete(expr);
      return klparser_error_oom(parser, lex);
    }
  }
  KlCstWhere* exprwhere = klcst_where_create(expr, block, klcst_begin(expr), klcst_end(block));
  klparser_oomifnull(exprwhere);
  return klcst(exprwhere);
}

static KlCst* klparser_finishappend(KlParser* parser, KlLex* lex) {
  KArray elemarr;
  if (kl_unlikely(!karray_init(&elemarr)))
    return klparser_error_oom(parser, lex);
  while (kllex_trymatch(lex, KLTK_APPEND)) {
    KlCst* unit = klparser_exprunit(parser, lex, NULL);
    if (kl_unlikely(!unit)) continue;
    klparser_karr_pushcst(&elemarr, unit);
  }
  karray_shrink(&elemarr);
  size_t nelem = karray_size(&elemarr);
  if (nelem == 0) {
    karray_destroy(&elemarr);
    return NULL;
  }
  KlCst** elems = (KlCst**)karray_steal(&elemarr);
  KlCstTuple* exprlist = klcst_tuple_create(elems, nelem, klcst_begin(elems[0]), klcst_end(elems[nelem - 1]));
  return klcst(exprlist);
}

KlCst* klparser_exprpost(KlParser* parser, KlLex* lex) {
  bool inparenthesis;
  KlCst* postexpr = klparser_exprunit(parser, lex, &inparenthesis);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LBRACKET: { /* index */
        if (kllex_hasleadingblank(lex)) {
          postexpr = klcst(klparser_exprcall(parser, lex, postexpr));
          break;
        }
        kllex_next(lex);
        KlCst* expr = klparser_expr(parser, lex);
        KlFileOffset end = lex->tok.end;
        klparser_match(parser, lex, KLTK_RBRACKET);
        if (kl_unlikely(!expr)) break;
        if (kl_unlikely(!postexpr)) {
          klcst_delete(expr);
          break;
        }
        KlCstPost* index = klcst_post_create(KLTK_INDEX, postexpr, expr, postexpr->begin, end);
        klparser_oomifnull(index);
        postexpr = klcst(index);
        break;
      }
      case KLTK_DARROW: {
        postexpr = inparenthesis ? klcst(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlCstStmtList* block = klparser_darrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klcst_delete(block);
          break;
        }
        bool vararg;
        KlCstTuple* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klcst_delete(block);
          return NULL;
        }
        KlCstFunc* func = klcst_func_create(block, params, vararg, false, klcst_begin(params), klcst_end(block));
        klparser_oomifnull(func);
        postexpr = klcst(func);
        break;
      }
      case KLTK_ARROW: {
        postexpr = inparenthesis ? klcst(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlCstStmtList* block = klparser_arrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klcst_delete(block);
          break;
        }
        bool vararg;
        KlCstTuple* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klcst_delete(block);
          return NULL;
        }
        KlCstFunc* func = klcst_func_create(block, params, vararg, false, klcst_begin(params), klcst_end(block));
        klparser_oomifnull(func);
        postexpr = klcst(func);
        break;
      }
      case KLTK_DOT: {
        kllex_next(lex);
        if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !postexpr))
          break;
        KlCstDot* dot = klcst_dot_create(postexpr, lex->tok.string, postexpr->begin, lex->tok.end);
        klparser_oomifnull(dot);
        postexpr = klcst(dot);
        kllex_next(lex);
        break;
      }
      case KLTK_APPEND: {
        KlCst* vals = klparser_finishappend(parser, lex);
        if (kl_unlikely(!postexpr || !vals)) {
          if (vals) klcst_delete(vals);
          break;
        }
        KlCstPost* arrpush = klcst_post_create(KLTK_APPEND, postexpr, vals, postexpr->begin, vals->end);
        klparser_oomifnull(arrpush);
        postexpr = klcst(arrpush);
        break;
      }
      case KLTK_ID:
      case KLTK_STRING:
      case KLTK_INT:
      case KLTK_FLOAT:
      case KLTK_BOOLVAL:
      case KLTK_NIL:
      case KLTK_VARARG:
      case KLTK_LPAREN:
      case KLTK_LBRACE: { /* function call */
        postexpr = klcst(klparser_exprcall(parser, lex, postexpr));
        break;
      }
      default: {  /* no more postfix, just return */
        return postexpr;
      }
    }
  }
}

KlCst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio) {
  static int binop_prio[KLTK_NTOKEN] = {
    [KLTK_OR]     = 1,
    [KLTK_AND]    = 2,
    [KLTK_LE]     = 3,
    [KLTK_LT]     = 3,
    [KLTK_GE]     = 3,
    [KLTK_GT]     = 3,
    [KLTK_EQ]     = 3,
    [KLTK_NE]     = 3,
    [KLTK_IS]     = 3,
    [KLTK_ISNOT]  = 3,
    [KLTK_CONCAT] = 4,
    [KLTK_ADD]    = 5,
    [KLTK_MINUS]  = 5,
    [KLTK_MUL]    = 6,
    [KLTK_DIV]    = 6,
    [KLTK_MOD]    = 6,
    [KLTK_IDIV]   = 6,
  };
  KlCst* left = klparser_exprpre(parser, lex);
  KlTokenKind op = kllex_tokkind(lex);
  while (kltoken_isbinop(op) && binop_prio[op] > prio) {
    kllex_next(lex);
    KlCst* right = klparser_exprbin(parser, lex, binop_prio[op]);
    if (kl_unlikely(!left || !right)) {
      if (left) klcst_delete(left);
      if (right) klcst_delete(right);
      return NULL;
    }
    KlCstBin* binexpr = klcst_bin_create(op, left, right, left->begin, right->end);
    klparser_oomifnull(binexpr);
    left = klcst(binexpr);
    op = kllex_tokkind(lex);
  }
  return klcst(left);
}


/* parser for statement */
static KlCst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex);

KlCstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFileOffset begin = lex->tok.begin;
    kllex_next(lex);
    KlCstStmtList* stmtlist = klparser_stmtlist(parser, lex);
    if (stmtlist) klcst_setposition(stmtlist, begin, lex->tok.end);
    klparser_match(parser, lex, KLTK_RBRACE);
    return stmtlist;
  } else {
    KlCst* stmt = klparser_stmt(parser, lex);
    klparser_returnifnull(stmt);
    KlCst** stmts = (KlCst**)malloc(1 * sizeof (KlCst*));
    if (kl_unlikely(!stmts)) {
      klcst_delete(stmt);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    KlCstStmtList* stmtlist = klcst_stmtlist_create(stmts, 1, stmt->begin, stmt->end);
    klparser_oomifnull(stmtlist);
    return stmtlist;
  }
}

KlCst* klparser_stmt(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_LET: {
      KlCst* stmtlet = klparser_stmtlet(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtlet;
    }
    case KLTK_IF: {
      KlCstStmtIf* stmtif = klparser_stmtif(parser, lex);
      return klcst(stmtif);
    }
    case KLTK_REPEAT: {
      KlCst* stmtrepeat = klparser_stmtrepeat(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtrepeat;
    }
    case KLTK_WHILE: {
      KlCst* stmtwhile = klparser_stmtwhile(parser, lex);
      return stmtwhile;
    }
    case KLTK_RETURN: {
      KlCst* stmtreturn = klparser_stmtreturn(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtreturn;
    }
    case KLTK_BREAK: {
      KlCst* stmtbreak = klparser_stmtbreak(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtbreak;
    }
    case KLTK_CONTINUE: {
      KlCst* stmtcontinue = klparser_stmtcontinue(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtcontinue;
    }
    case KLTK_FOR: {
      KlCst* stmtfor = klparser_stmtfor(parser, lex);
      return stmtfor;
    }
    default: {
      KlCst* res = klparser_stmtexprandassign(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return res;
    }
  }
}

static KlCst* klparser_stmt_nosemi(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_LET: {
      KlCst* stmtlet = klparser_stmtlet(parser, lex);
      return stmtlet;
    }
    case KLTK_IF: {
      KlCstStmtIf* stmtif = klparser_stmtif(parser, lex);
      return klcst(stmtif);
    }
    case KLTK_REPEAT: {
      KlCst* stmtrepeat = klparser_stmtrepeat(parser, lex);
      return stmtrepeat;
    }
    case KLTK_WHILE: {
      KlCst* stmtwhile = klparser_stmtwhile(parser, lex);
      return stmtwhile;
    }
    case KLTK_RETURN: {
      KlCst* stmtreturn = klparser_stmtreturn(parser, lex);
      return stmtreturn;
    }
    case KLTK_BREAK: {
      KlCst* stmtbreak = klparser_stmtbreak(parser, lex);
      return stmtbreak;
    }
    case KLTK_CONTINUE: {
      KlCst* stmtcontinue = klparser_stmtcontinue(parser, lex);
      return stmtcontinue;
    }
    case KLTK_FOR: {
      KlCst* stmtfor = klparser_stmtfor(parser, lex);
      return stmtfor;
    }
    default: {
      KlCst* res = klparser_stmtexprandassign(parser, lex);
      return res;
    }
  }
}
static KlCst* klparser_discoveryforblock(KlParser* parser, KlLex* lex) {
  klparser_match(parser, lex, KLTK_COLON);
  KlCstStmtList* block = klparser_stmtblock(parser, lex);
  if (block) klcst_delete(block);
  return NULL;
}

static inline KlCstStmtList* klparser_finishstmtfor(KlParser* parser, KlLex* lex) {
  kllex_trymatch(lex, KLTK_COLON);
  return klparser_stmtblock(parser, lex);
}

static KlCst* klparser_stmtinfor(KlParser* parser, KlLex* lex) {
  KlCstTuple* lvals = klcast(KlCstTuple*, klparser_exprlist(parser, lex));
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* stmtfor -> for i = n, m, s */
    KlCstTuple* exprlist = klparser_exprlist(parser, lex);
    if (kl_unlikely(!exprlist)) {
      if (lvals) klcst_delete(lvals);
      return klparser_discoveryforblock(parser, lex);
    }
    KlCstStmtList* block = klparser_finishstmtfor(parser, lex);
    if (kl_unlikely(!block)) {
      if (lvals) klcst_delete(lvals);
      klcst_delete(exprlist);
      return NULL;
    }
    if (kl_unlikely(!lvals)) {
      klcst_delete(exprlist);
      klcst_delete(block);
      return NULL;
    }
    if (kl_unlikely(lvals->nelem != 1))
      klparser_error(parser, kllex_inputstream(lex), klcst_begin(lvals), klcst_end(lvals), "integer loop requires only one iteration variable");
    size_t nelem = klcast(KlCstTuple*, exprlist)->nelem;
    if (kl_unlikely(nelem != 3 && nelem != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klcst_begin(exprlist), klcst_end(exprlist), "expect 2 or 3 expressions here");
      klcst_delete(lvals);
      klcst_delete(block);
      klcst_delete(exprlist);
      return NULL;
    }
    KlCst** exprs = klcast(KlCstTuple*, exprlist)->elems;
    KlCstStmtIFor* ifor = klcst_stmtifor_create(klcst(lvals), exprs[0], exprs[1], nelem == 3 ? exprs[2] : NULL, block, ph_filepos, klcst_end(block));
    klcst_tuple_shallow_replace(klcast(KlCstTuple*, exprlist), NULL, 0);
    klcst_delete(exprlist);
    klparser_oomifnull(ifor);
    return klcst(ifor);
  }
  /* stmtfor -> for a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlCst* iterable = klparser_expr(parser, lex);
  if (kl_unlikely(!iterable)) {
    if (lvals) klcst_delete(lvals);
    return klparser_discoveryforblock(parser, lex);
  }
  KlCstStmtList* block = klparser_finishstmtfor(parser, lex);
  if (kl_unlikely(!block)) {
    if (lvals) klcst_delete(lvals);
    klcst_delete(iterable);
    return NULL;
  }
  if (kl_unlikely(!lvals)) {
    klcst_delete(iterable);
    klcst_delete(block);
    return NULL;
  }
  kl_assert(klcast(KlCstTuple*, lvals)->nelem != 0, "");
  if (klcst_kind(iterable) == KLCST_EXPR_VARARG) {
    /* is variable argument for loop : stmtfor -> for a, b, ... in ... */
    klcst_delete(iterable); /* 'iterable' is no longer needed */
    KlCstStmtVFor* vfor = klcst_stmtvfor_create(lvals, block, ph_filepos, klcst_end(block));
    klparser_oomifnull(vfor);
    return klcst(vfor);
  } else {  /* generic for loop : stmtfor -> for a, b, ... in expr */
    KlCstStmtGFor* gfor = klcst_stmtgfor_create(lvals, iterable, block, ph_filepos, klcst_end(block));
    klparser_oomifnull(gfor);
    return klcst(gfor);
  }
}

KlCst* klparser_stmtlet(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LET), "expect 'let'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  KlCstTuple* lvals = klcast(KlCstTuple*, klparser_exprlist(parser, lex));
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlCstTuple* exprlist = klparser_exprlist(parser, lex);
  if (kl_unlikely(!exprlist || !lvals)) {
    if (lvals) klcst_delete(lvals);
    if (exprlist) klcst_delete(exprlist);
    return NULL;
  }
  KlCstStmtLet* stmtlet = klcst_stmtlet_create(lvals, exprlist, begin, klcst_end(exprlist));
  klparser_oomifnull(stmtlet);
  return klcst(stmtlet);
}

static KlCst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex) {
  KlCstTuple* maybelvals = klparser_exprlist(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* is assignment */
    KlCstTuple* rvals = klparser_exprlist(parser, lex);
    if (kl_unlikely(!rvals || !maybelvals)) {
      if (rvals) klcst_delete(rvals);
      if (maybelvals) klcst_delete(maybelvals);
      return NULL;
    }
    KlCstStmtAssign* stmtassign = klcst_stmtassign_create(maybelvals, rvals, klcst_begin(maybelvals), klcst_end(rvals));
    klparser_oomifnull(stmtassign);
    return klcst(stmtassign);
  } else {
    klparser_returnifnull(maybelvals);
    /* just an expression list statement */
    KlCstStmtExpr* stmtexpr = klcst_stmtexpr_create(maybelvals, klcst_begin(maybelvals), klcst_end(maybelvals));
    klparser_oomifnull(stmtexpr);
    return klcst(stmtexpr);
  }
}

KlCstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_IF), "expect 'if'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* cond = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlCstStmtList* then_block = NULL;
  then_block = klparser_stmtblock(parser, lex);
  KlCstStmtList* else_block = NULL;
  if (kllex_trymatch(lex, KLTK_ELSE)) {     /* else block */
    kllex_trymatch(lex, KLTK_COLON);
    else_block = klparser_stmtblock(parser, lex);
  }
  if (kl_unlikely(!then_block || !cond)) {
    if (cond) klcst_delete(cond);
    if (then_block) klcst_delete(then_block);
    return NULL;
  }
  KlCstStmtIf* stmtif = klcst_stmtif_create(cond, then_block, else_block, begin, else_block ? klcst_end(else_block): klcst_end(then_block));
  klparser_oomifnull(stmtif);
  return stmtif;
}

KlCst* klparser_stmtfor(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_FOR), "expect '('");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* stmtfor = klparser_stmtinfor(parser, lex);
  klparser_returnifnull(stmtfor);
  klcst_setposition(klcast(KlCst*, stmtfor), begin, stmtfor->end);
  return stmtfor;
}

KlCst* klparser_stmtwhile(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_WHILE), "expect 'while'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* cond = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_COLON);
  KlCstStmtList* block = klparser_stmtblock(parser, lex);
  if (kl_unlikely(!cond || !block)) {
    if (cond) klcst_delete(cond);
    if (block) klcst_delete(block);
    return NULL;
  }
  KlCstStmtWhile* stmtwhile = klcst_stmtwhile_create(cond, block, begin, klcst_end(block));
  klparser_oomifnull(stmtwhile);
  return klcst(stmtwhile);
}

KlCstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex) {
  KArray stmts;
  if (kl_unlikely(!karray_init(&stmts)))
    return klparser_error_oom(parser, lex);
  while (true) {
    switch (kllex_tokkind(lex)) {
      default: {
        if(!klparser_exprbegin(lex)) break;
        kl_fallthrough;
      }
      case KLTK_LET: case KLTK_IF: case KLTK_REPEAT: case KLTK_WHILE:
      case KLTK_FOR: case KLTK_RETURN: case KLTK_BREAK: case KLTK_CONTINUE: {
        KlCst* stmt = klparser_stmt(parser, lex);
        if (kl_unlikely(!stmt)) continue;
        if (kl_unlikely(!karray_push_back(&stmts, stmt))) {
          klcst_delete(stmt);
          klparser_error_oom(parser, lex);
        }
        continue;
      }
    }
    break;
  }
  karray_shrink(&stmts);
  size_t nstmt = karray_size(&stmts);
  KlCstStmtList* stmtlist = klcst_stmtlist_create((KlCst**)karray_steal(&stmts), nstmt, ph_filepos, ph_filepos);
  klparser_oomifnull(stmtlist);
  return stmtlist;
}

KlCst* klparser_stmtrepeat(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_REPEAT), "expect 'repeat'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlCstStmtList* block = klparser_stmtblock(parser, lex);
  klparser_match(parser, lex, KLTK_UNTIL);
  KlCst* cond = klparser_expr(parser, lex);
  if (kl_unlikely(!cond || !block)) {
    if (cond) klcst_delete(cond);
    if (block) klcst_delete(block);
    return NULL;
  }
  KlCstStmtRepeat* stmtrepeat = klcst_stmtrepeat_create(block, cond, begin, cond->end);
  klparser_oomifnull(stmtrepeat);
  return klcast(KlCst*, stmtrepeat);
}

KlCst* klparser_stmtreturn(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_RETURN), "expect 'return'");
  KlFileOffset begin = lex->tok.begin;
  kllex_next(lex);
  KlCstTuple* results = klparser_exprlist_mayempty(parser, lex);
  klparser_returnifnull(results);
  KlCstStmtReturn* stmtreturn = klcst_stmtreturn_create(results, begin, klcst_end(results));
  klparser_oomifnull(stmtreturn);
  return klcst(stmtreturn);
}

static KlCst* klparser_generatorfor(KlParser* parser, KlLex* lex, KlCstTuple* tuple, KlCst* inner_stmt) {
  KlCstTuple* lvals = klcast(KlCstTuple*, tuple);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* i = n, m, s */
    KlCstTuple* exprlist = klparser_exprlist(parser, lex);
    kllex_trymatch(lex, KLTK_SEMI);
    if (kl_unlikely(!exprlist)) {
      klcst_delete(exprlist);
      klcst_delete(lvals);
      klcst_delete(inner_stmt);
      return NULL;
    }
    size_t nelem = klcast(KlCstTuple*, exprlist)->nelem;
    if (kl_unlikely(nelem != 3 && nelem != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klcst_begin(exprlist), klcst_end(exprlist), "expect 2 or 3 expressions here");
      klcst_delete(exprlist);
      klcst_delete(inner_stmt);
      klcst_delete(lvals);
      return NULL;
    }
    if (kl_unlikely(lvals->nelem != 1)) {
      klparser_error(parser, kllex_inputstream(lex), klcst_begin(lvals), klcst_end(lvals), "integer loop requires only one iteration variable");
      klcst_delete(exprlist);
      klcst_delete(inner_stmt);
      klcst_delete(lvals);
      return NULL;
    }
    KlCstStmtList* block = klparser_generator(parser, lex, inner_stmt);
    if (kl_unlikely(!block)) {
      klcst_delete(exprlist);
      klcst_delete(lvals);
      return NULL;
    }
    KlCst** exprs = klcast(KlCstTuple*, exprlist)->elems;
    KlCstStmtIFor* ifor = klcst_stmtifor_create(klcst(lvals), exprs[0], exprs[1], nelem == 3 ? exprs[2] : NULL, block, klcst_begin(lvals), klcst_end(block));
    klcst_tuple_shallow_replace(klcast(KlCstTuple*, exprlist), NULL, 0);
    klcst_delete(exprlist);
    klparser_oomifnull(ifor);
    return klcst(ifor);
  }
  /* a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlCst* iterable = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_SEMI);
  if (kl_unlikely(!iterable)) {
    klcst_delete(inner_stmt);
    klcst_delete(lvals);
    return NULL;
  }
  KlCstStmtList* block = klparser_generator(parser, lex, inner_stmt);
  if (kl_unlikely(!block)) {
    klcst_delete(lvals);
    klcst_delete(iterable);
    return NULL;
  }
  if (klcst_kind(iterable) == KLCST_EXPR_VARARG) {  /* a, b, ... in ... */
    klcst_delete(iterable); /* 'iterable' is no longer needed */
    KlCstStmtVFor* vfor = klcst_stmtvfor_create(lvals, block, klcst_begin(lvals), klcst_end(block));
    klparser_oomifnull(vfor);
    return klcst(vfor);
  } else {  /* a, b, ... in expr */
    KlCstStmtGFor* gfor = klcst_stmtgfor_create(lvals, iterable, block, klcst_begin(lvals), klcst_end(block));
    klparser_oomifnull(gfor);
    return klcst(gfor);
  }
}

KlCstStmtList* klparser_generator(KlParser* parser, KlLex* lex, KlCst* inner_stmt) {
  KArray stmtarr;
  if (kl_unlikely(!karray_init(&stmtarr)))
    return klparser_error_oom(parser, lex);
  while (true) {
    if (klparser_exprbegin(lex)) {
      KlCstTuple* exprlist = klparser_exprlist(parser, lex);
      if (kl_unlikely(!exprlist)) {
        kllex_trymatch(lex, KLTK_SEMI);
        continue;
      }
      if (kllex_check(lex, KLTK_IN) || kllex_check(lex, KLTK_ASSIGN)) {
        KlCst* stmt = klparser_generatorfor(parser, lex, exprlist, inner_stmt);
        /* 'tuple' is deleted in klparser_generatorfor() */
        if (kl_unlikely(!stmt)) {
          klparser_destroy_cstarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmt))) {
          klparser_error_oom(parser, lex);
          klcst_delete(klcst(stmt));
          klparser_destroy_cstarray(&stmtarr);
          return NULL;
        }
      } else {
        kllex_trymatch(lex, KLTK_SEMI);
        KlCstStmtList* block = klparser_generator(parser, lex, inner_stmt);
        if (kl_unlikely(!block)) {
          klcst_delete(exprlist);
          klparser_destroy_cstarray(&stmtarr);
          return NULL;
        }
        KlCstStmtIf* stmtif = klcst_stmtif_create(klcst(exprlist), block, NULL, klcst_begin(exprlist), klcst_end(block));
        if (kl_unlikely(!stmtif)) {
          klparser_destroy_cstarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmtif))) {
          klcst_delete(klcst(stmtif));
          klparser_destroy_cstarray(&stmtarr);
          return klparser_error_oom(parser, lex);
        }
      }
      kl_assert(karray_size(&stmtarr) != 0, "");
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlCst** stmts = (KlCst**)karray_steal(&stmtarr);
      KlCstStmtList* stmtlist = klcst_stmtlist_create(stmts, nstmt, stmts[0]->begin, stmts[nstmt - 1]->end);
      klparser_oomifnull(stmtlist);
      return stmtlist;
    } else if (kllex_check(lex, KLTK_LET)) {
      KlCst* stmtlet = klparser_stmtlet(parser, lex);
      if (kl_unlikely(!stmtlet)) continue;
      if (kl_unlikely(!karray_push_back(&stmtarr, stmtlet))) {
        klcst_delete(stmtlet);
        klparser_error_oom(parser, lex);
      }
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      KlFileOffset begin = karray_size(&stmtarr) == 0 ? inner_stmt->begin : klcst_begin(karray_access(&stmtarr, 0));
      KlFileOffset end = karray_size(&stmtarr) == 0 ? inner_stmt->end : klcst_end(karray_access(&stmtarr, karray_size(&stmtarr) - 1));
      if (kl_unlikely(!karray_push_back(&stmtarr, inner_stmt))) {
        klparser_destroy_cstarray(&stmtarr);
        klcst_delete(inner_stmt);
        return klparser_error_oom(parser, lex);
      }
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlCst** stmts = (KlCst**)karray_steal(&stmtarr);
      KlCstStmtList* stmtlist = klcst_stmtlist_create(stmts, nstmt, begin, end);
      klparser_oomifnull(stmtlist);
      return stmtlist;
    }
  }
}


/* error handler */
void klparser_error(KlParser* parser, Ki* input, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  kl_assert(begin != ph_filepos && end != ph_filepos, "position of a syntax tree not set!");
  va_list args;
  va_start(args, format);
  klerror_errorv(parser->klerror, input, parser->inputname, begin, end, format, args);
  va_end(args);
}
