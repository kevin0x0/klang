#include "include/parse/klparser.h"
#include "include/parse/klcfdarr.h"
#include "include/parse/klidarr.h"
#include "include/parse/kllex.h"
#include "include/parse/kltokens.h"
#include "include/error/klerror.h"
#include "include/ast/klast.h"
#include "include/misc/klutils.h"
#include "deps/k/include/array/karray.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


static KlFileOffset ph_filepos = ~(KlFileOffset)0;


static void klparser_error(KlParser* parser, Ki* input, KlFileOffset begin, KlFileOffset end, const char* format, ...);
static void* klparser_error_oom(KlParser* parser, KlLex* lex);

static bool klparser_discarduntil(KlLex* lex, KlTokenKind kind);
static bool klparser_discardto(KlLex* lex, KlTokenKind kind);

static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind);
/* check whether current token match 'kind', if not, report an error and try to discover.
 * if discovery failed, return false. */
static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind);




/* parser for expression */
static KlAstStmtList* klparser_generator(KlParser* parser, KlLex* lex, KlAst* inner_stmt);
static KlAstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex);
static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex);
static KlAstExprList* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end);
static KlAst* klparser_finishmap(KlParser* parser, KlLex* lex);
static KlAst* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAst* expr, bool preparsed);
static KlAst* klparser_generatorclass(KlParser* parser, KlLex* lex);
static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static void klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static KlAst* klparser_array(KlParser* parser, KlLex* lex);
static KlAstExprList* klparser_finishexprlist(KlParser* parser, KlLex* lex, KlAst* expr);
static KlAst* klparser_dotchain(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprnew(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprwhere(KlParser* parser, KlLex* lex, KlAst* expr);
static KlAst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis);
static KlAst* klparser_exprpost(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprpre(KlParser* parser, KlLex* lex);
static KlAst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio);

static inline bool klparser_exprbegin(KlLex* lex);

/* parser for statement */
static KlAst* klparser_stmt_nosemi(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtlet(KlParser* parser, KlLex* lex);
static KlAstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtfor(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtwhile(KlParser* parser, KlLex* lex);
/* do not set file position */
static KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex);
/* do not set file position */
static KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtrepeat(KlParser* parser, KlLex* lex);
static KlAst* klparser_stmtreturn(KlParser* parser, KlLex* lex);
static inline KlAst* klparser_stmtbreak(KlParser* parser, KlLex* lex);
static inline KlAst* klparser_stmtcontinue(KlParser* parser, KlLex* lex);




bool klparser_init(KlParser* parser, KlStrTbl* strtbl, const char* inputname, KlError* klerror) {
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
  char* newid = klstrtbl_allocstring(parser->strtbl, sizeof (unsigned) * CHAR_BIT);
  if (kl_unlikely(!newid)) {
    klparser_error_oom(parser, lex);
    return (KlStrDesc) { .id = 0, .length = 0 };
  }
  newid[0] = '\0';  /* all temporary identifiers begin with '\0' */
  int len = sprintf(newid + 1, "%u", parser->incid++) + 1;
  size_t strid = klstrtbl_pushstring(parser->strtbl, len);
  return (KlStrDesc) { .id = strid, .length = len };
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

static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expect '%s'", kltoken_desc(kind));
    return klparser_discardto(lex, kind);
  }
  kllex_next(lex);
  return true;
}

static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expect '%s'", kltoken_desc(kind));
    return klparser_discarduntil(lex, kind);
  }
  return true;
}

static void* klparser_error_oom(KlParser* parser, KlLex* lex) {
  klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "out of memory");
  return NULL;
}



#define klparser_karr_pushast(arr, ast)  {              \
  if (kl_unlikely(!karray_push_back(arr, ast))) {       \
    klparser_error_oom(parser, lex);                    \
    if (ast) klast_delete(ast);                         \
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

KlAst* klparser_expr(KlParser* parser, KlLex* lex) {
  KlAst* expr = klparser_exprbin(parser, lex, 0);
  if (kl_unlikely(!expr)) return NULL;
  return kllex_check(lex, KLTK_WHERE) ? klparser_exprwhere(parser, lex, expr) :
                                        expr;
}

static inline KlAstExprList* klparser_exprlist(KlParser* parser, KlLex* lex) {
  KlAst* headexpr = klparser_expr(parser, lex);
  return headexpr == NULL ? NULL : klparser_finishexprlist(parser, lex, headexpr);
}

static KlAstExprList* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end) {
  KlAstExprList* exprlist = klast_exprlist_create(NULL, 0, begin, end);
  if (kl_unlikely(!exprlist)) return klparser_error_oom(parser, lex);
  return exprlist;
}

static inline KlAstExprList* klparser_exprlist_mayempty(KlParser* parser, KlLex* lex) {
  return klparser_exprbegin(lex) ? klparser_exprlist(parser, lex) : klparser_emptyexprlist(parser, lex, kllex_tokbegin(lex), kllex_tokbegin(lex));
}

static KlAstExprList* klparser_singletonexprlist(KlParser* parser, KlLex* lex, KlAst* expr) {
  KlAst** elems = (KlAst**)malloc(sizeof (KlAst*));
  if (kl_unlikely(!elems)) {
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  elems[0] = expr;
  KlAstExprList* exprlist = klast_exprlist_create(elems, 1, expr->begin, expr->end);
  if (kl_unlikely(!exprlist))
    return klparser_error_oom(parser, lex);
  return exprlist;
}

/* tool for clean ast karray when error occurred. */
static void klparser_destroy_astarray(KArray* arr) {
  for (size_t i = 0; i < karray_size(arr); ++i) {
    KlAst* ast = (KlAst*)karray_access(arr, i);
    /* ast may be NULL when error occurred */
    if (ast) klast_delete(ast);
  }
  karray_destroy(arr);
}

static KlAst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis) {
  if (inparenthesis) *inparenthesis = false;
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlAstConstant* ast = klast_constant_create_integer(lex->tok.intval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_FLOAT: {
      KlAstConstant* ast = klast_constant_create_float(lex->tok.floatval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_STRING: {
      KlAstConstant* ast = klast_constant_create_string(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_BOOLVAL: {
      KlAstConstant* ast = klast_constant_create_boolean(lex->tok.boolval, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_NIL: {
      KlAstConstant* ast = klast_constant_create_nil(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_ID: {
      KlAstIdentifier* ast = klast_id_create(lex->tok.string, kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_VARARG: {
      KlAstVararg* ast = klast_vararg_create(kllex_tokbegin(lex), kllex_tokend(lex));
      if (kl_unlikely(!ast)) return klparser_error_oom(parser, lex);
      kllex_next(lex);
      return klast(ast);
    }
    case KLTK_LBRACKET: {
      return klast(klparser_array(parser, lex));
    }
    case KLTK_LBRACE: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* ast = kllex_check(lex, KLTK_COLON) ? klparser_finishmap(parser, lex) : klparser_generatorclass(parser, lex);
      KlFileOffset end = kllex_tokend(lex);
      klparser_match(parser, lex, KLTK_RBRACE);
      if (kl_likely(ast)) klast_setposition(ast, begin, end);
      return ast;
    }
    case KLTK_LPAREN: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlFileOffset mayend = kllex_tokend(lex);
      if (kllex_trymatch(lex, KLTK_RPAREN)) { /* empty exprlist */
        KlAstExprList* exprlist = klparser_emptyexprlist(parser, lex, begin, mayend);
        return klast(exprlist);
      }
      KlAst* expr = klparser_expr(parser, lex);
      if (kllex_check(lex, KLTK_RPAREN)) {  /* not an exprlist */
        if (inparenthesis) *inparenthesis = true;
        klast_setposition(expr, begin, kllex_tokend(lex));
        kllex_next(lex);
        return expr;
      }
      /* is a exprlist */
      klparser_returnifnull(expr);
      KlAstExprList* exprlist = klparser_finishexprlist(parser, lex, expr);
      KlFileOffset end = kllex_tokend(lex);
      klparser_match(parser, lex, KLTK_RPAREN);
      klparser_returnifnull(exprlist);
      klast_setposition(exprlist, begin, end);
      return klast(exprlist);
    }
    default: {
      klparser_error(parser, kllex_inputstream(lex), kllex_tokbegin(lex), kllex_tokend(lex),
                     "expect '(', '{', '[', true, false, identifier, string or integer");
      return NULL;
    }
  }
}

static KlAst* klparser_array(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LBRACKET), "expect \'[\'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);  /* skip '[' */
  KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discardto(lex, KLTK_RBRACKET);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_BAR)) { /* array generator */
    KlAstIdentifier* arrid = klast_id_create(klparser_newtmpid(parser, lex), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(arrid);
    KlAstPost* exprpush = klast_post_create(KLTK_APPEND, klast(arrid), klast(exprlist), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(exprpush);
    KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klast(exprpush));
    klparser_oomifnull(single_expr);
    KlAstStmtExpr* inner_stmt = klast_stmtexpr_create(single_expr, klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(inner_stmt);
    KlAstStmtList* stmtlist = klparser_generator(parser, lex, klcast(KlAst*, inner_stmt));
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    klparser_returnifnull(stmtlist);  /* 'inner_stmt' will be deleted in klparser_generator() when error occurred */
    KlAstArrayGenerator* array = klast_arraygenerator_create(arrid->id, stmtlist, begin, end);
    return klast(array);
  } else {
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACKET);
    KlAstArray* array = klast_array_create(exprlist, begin, end);
    klparser_oomifnull(array);
    return klast(array);
  }
}

static KlAstExprList* klparser_finishexprlist(KlParser* parser, KlLex* lex, KlAst* expr) {
  KArray exprs;
  if (kl_unlikely(!karray_init(&exprs)))
    return klparser_error_oom(parser, lex);
  if (kl_unlikely(!karray_push_back(&exprs, expr))) {
    klparser_destroy_astarray(&exprs);
    klast_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  while (kllex_trymatch(lex, KLTK_COMMA) && klparser_exprbegin(lex)) {
    KlAst* expr = klparser_expr(parser, lex);
    if (kl_unlikely(!expr)) continue;
    klparser_karr_pushast(&exprs, expr);
  }
  karray_shrink(&exprs);
  size_t nelem = karray_size(&exprs);
  KlAstExprList* exprlist = klast_exprlist_create((KlAst**)karray_steal(&exprs), nelem, expr->begin, klast_end(karray_top(&exprs)));
  klparser_oomifnull(exprlist);
  return exprlist;
}

static KlAst* klparser_finishmap(KlParser* parser, KlLex* lex) {
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
      KlAst* key = klparser_expr(parser, lex);
      klparser_match(parser, lex, KLTK_COLON);
      KlAst* val = klparser_expr(parser, lex);
      if (kl_likely(key && val)) {
        klparser_karr_pushast(&keys, key);
        klparser_karr_pushast(&vals, val);
      } else {
        if (key) klast_delete(key);
        if (val) klast_delete(val);
      }
    } while (kllex_trymatch(lex, KLTK_COMMA));
  }
  klparser_match(parser, lex, KLTK_COLON);
  if (kl_unlikely(karray_size(&keys) != karray_size(&vals))) {
    klparser_destroy_astarray(&keys);
    klparser_destroy_astarray(&vals);
    return NULL;
  }
  karray_shrink(&keys);
  karray_shrink(&vals);
  size_t npair = karray_size(&keys);
  KlAstMap* map = klast_map_create((KlAst**)karray_steal(&keys), (KlAst**)karray_steal(&vals), npair, ph_filepos, ph_filepos);
  klparser_oomifnull(map);
  return klast(map);
}

static KlAst* klparser_finishclass(KlParser* parser, KlLex* lex, KlStrDesc id, KlAst* expr, bool preparsed) {
  KlCfdArray fields;
  KArray vals;
  if (kl_unlikely(!klcfd_init(&fields, 16) || !karray_init(&vals))) {
    klcfd_destroy(&fields);
    karray_destroy(&vals);
    return klparser_error_oom(parser, lex);
  }
  if (preparsed) {
    /* insert first shared k-v pair */
    KlAstClassFieldDesc cfd;
    cfd.shared = true;
    cfd.name = id;
    if (kl_unlikely(!klcfd_push_back(&fields, &cfd)))
      klparser_error_oom(parser, lex);
    if (kl_unlikely(!karray_push_back(&vals, expr))) {
      if (expr) klast_delete(expr);
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
      klparser_error(parser, kllex_inputstream(lex), kllex_tokbegin(lex), kllex_tokend(lex), "end of file");
      break;
    }
  }
  if (kl_unlikely(klcfd_size(&fields) != karray_size(&vals))) {
    klcfd_destroy(&fields);
    klparser_destroy_astarray(&vals);
    return NULL;
  }
  klcfd_shrink(&fields);
  karray_shrink(&vals);
  size_t nfield = karray_size(&vals);
  KlAstClass* klclass = klast_class_create(klcfd_steal(&fields), (KlAst**)karray_steal(&vals), nfield, NULL, ph_filepos, ph_filepos);
  klparser_oomifnull(klclass);
  return klast(klclass);
}

static KlAst* klparser_generatorclass(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_RBRACE) ||
      kllex_check(lex, KLTK_LOCAL)  ||
      kllex_check(lex, KLTK_SHARED)) {  /* is class */
    KlStrDesc id = { 0, 0 };  /* the value is unimportant here */
    return klparser_finishclass(parser, lex, id, NULL, false);
  }

  KlAstExprList* exprlist = klparser_exprlist(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klparser_discarduntil(lex, KLTK_RBRACE);
    return NULL;
  }
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* a class? */
    if (kl_unlikely(exprlist->nexpr != 1                            ||
                    klast_kind(exprlist->exprs[0]) != KLAST_EXPR_ID)) {
      klparser_error(parser, kllex_inputstream(lex),
                     klast_begin(exprlist), klast_end(exprlist),
                     "should be a single identifier in class definition");
      klast_delete(klcast(KlAst*, exprlist));
      klparser_discarduntil(lex, KLTK_RBRACE);
      return NULL;
    }
    KlStrDesc id = klcast(KlAstIdentifier*, exprlist->exprs[0])->id;
    klast_delete(klcast(KlAst*, exprlist));
    KlAst* expr = klparser_expr(parser, lex);
    return klparser_finishclass(parser, lex, id, expr, true);
  }
  /* else is a generator */
  klparser_match(parser, lex, KLTK_BAR);
  KlAstYield* yieldexpr = klast_yield_create(exprlist, klast_begin(exprlist), klast_end(exprlist));
  klparser_oomifnull(yieldexpr);
  KlAstExprList* single_expr = klparser_singletonexprlist(parser, lex, klast(yieldexpr));
  klparser_oomifnull(single_expr);
  KlAstStmtExpr* stmtexpr = klast_stmtexpr_create(single_expr, klast_begin(yieldexpr),  klast_end(yieldexpr));
  klparser_oomifnull(stmtexpr);
  KlAstStmtList* generator = klparser_generator(parser, lex, klast(stmtexpr));
  klparser_returnifnull(generator);
  KlAstExprList* params = klparser_emptyexprlist(parser, lex, klast_begin(stmtexpr), klast_begin(stmtexpr));
  if (kl_unlikely(!params)) {
    klast_delete(generator);
    return NULL;
  }
  KlAstFunc* func = klast_func_create(generator, params, false, false, klast_begin(stmtexpr), klast_end(generator));
  klparser_oomifnull(func);
  KlAstPre* asyncexpr = klast_pre_create(KLTK_ASYNC, klast(func), ph_filepos, ph_filepos);
  klparser_oomifnull(asyncexpr);
  return klast(asyncexpr);
}

static void klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlAstClassFieldDesc fielddesc;
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
  KlAstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return;
    fielddesc.name = lex->tok.string;
    kllex_next(lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlAst* val = klparser_expr(parser, lex);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc)))
      klparser_error_oom(parser, lex);
    klparser_karr_pushast(vals, val);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return;
}

static KlAst* klparser_dotchain(KlParser* parser, KlLex* lex) {
  KlAst* dotexpr = klparser_exprunit(parser, lex, NULL);
  while (kllex_trymatch(lex, KLTK_DOT)) {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      continue;
    if (kl_unlikely(!dotexpr)) {
      kllex_next(lex);
      continue;
    }
    KlAstDot* dot = klast_dot_create(dotexpr, lex->tok.string, dotexpr->begin, kllex_tokend(lex));
    klparser_oomifnull(dot);
    dotexpr = klast(dot);
    kllex_next(lex);
  }
  return dotexpr;
}

static KlAst* klparser_exprnew(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_NEW), "");

  kllex_next(lex);
  KlAst* klclass = kllex_check(lex, KLTK_LPAREN) ? klparser_exprunit(parser, lex, NULL) : klparser_dotchain(parser, lex);
  kltodo("must have (patameters)");
  KlAst* args = klparser_exprunit(parser, lex, NULL);
  if (kl_unlikely(!klclass || !args)) {
    if (klclass) klast_delete(klclass);
    if (args) klast_delete(args);
    return NULL;
  }
  KlAstNew* newexpr = klast_new_create(klclass, klcast(KlAstExprList*, args), klclass->begin, args->end);
  klparser_oomifnull(newexpr);
  return klast(newexpr);
}

static KlAst* klparser_exprpre(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* neg = klast_pre_create(KLTK_MINUS, expr, begin, expr->end);
      klparser_oomifnull(neg);
      return klast(neg);
    }
    case KLTK_NOT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* notexpr = klast_pre_create(KLTK_NOT, expr, begin, expr->end);
      klparser_oomifnull(notexpr);
      return klast(notexpr);
    }
    case KLTK_ARROW: {
      KlFileOffset begin = kllex_tokbegin(lex);
      KlAstStmtList* block = klparser_arrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlAstExprList* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, false, false, begin, klast_end(block));
      klparser_oomifnull(func);
      return klast(func);
    }
    case KLTK_DARROW: {
      KlFileOffset begin = kllex_tokbegin(lex);
      KlAstStmtList* block = klparser_darrowfuncbody(parser, lex);
      klparser_returnifnull(block);
      KlAstExprList* params = klparser_emptyexprlist(parser, lex, begin, begin);
      if (kl_unlikely(!params)) {
        klast_delete(block);
        return NULL;
      }
      KlAstFunc* func = klast_func_create(block, params, false, false, begin, klast_end(block));
      klparser_oomifnull(func);
      return klast(func);
    }
    case KLTK_YIELD: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAstExprList* exprlist = klparser_exprlist_mayempty(parser, lex);
      klparser_returnifnull(exprlist);
      KlAstYield* yieldexpr = klast_yield_create(exprlist, begin, klast_end(exprlist));
      klparser_oomifnull(yieldexpr);
      return klast(yieldexpr);
    }
    case KLTK_ASYNC: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      KlAstPre* asyncexpr = klast_pre_create(KLTK_ASYNC, expr, begin, expr->end);
      klparser_oomifnull(asyncexpr);
      return klast(asyncexpr);
    }
    case KLTK_INHERIT: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* base = klparser_exprpre(parser, lex);
      klparser_returnifnull(base);
      klparser_match(parser, lex, KLTK_COLON);
      KlAst* ast = klparser_exprunit(parser, lex, NULL);
      if (kl_unlikely(!ast)) {
        klast_delete(base);
        return NULL;
      }
      if (kl_unlikely(klast_kind(ast) != KLAST_EXPR_CLASS)) {
        klparser_error(parser, kllex_inputstream(lex), ast->begin, ast->end,
                       "must be a class definition");
        klast_delete(base);
        klast_delete(ast);
        return NULL;
      }
      KlAstClass* klclass = klcast(KlAstClass*, ast);
      klclass->baseclass = base;
      klast_setposition(klclass, begin, klast_end(klclass));
      return klast(klclass);
    }
    case KLTK_METHOD: {
      KlFileOffset begin = kllex_tokbegin(lex);
      kllex_next(lex);
      KlAst* expr = klparser_exprpre(parser, lex);
      klparser_returnifnull(expr);
      if (kl_unlikely(klast_kind(expr) != KLAST_EXPR_FUNC)) {
        klparser_error(parser, kllex_inputstream(lex), expr->begin, expr->end,
                       "'method' must be followed by a function construction");
        return expr;
      }
      KlAst* idthis = klast(klast_id_create(parser->string.this, klast_begin(expr), klast_begin(expr)));
      if (kl_unlikely(!idthis)) {
        klast_delete(expr);
        return klparser_error_oom(parser, lex);
      }
      KlAstFunc* func = klcast(KlAstFunc*, expr);
      KlAstExprList* funcparams = func->params;
      KlAst** newarr = (KlAst**)realloc(funcparams->exprs, (funcparams->nexpr + 1) * sizeof (KlAst*));
      if (kl_unlikely(!newarr)) {
        klast_delete(expr);
        klast_delete(idthis);
        return klparser_error_oom(parser, lex);
      }
      memmove(newarr + 1, newarr, funcparams->nexpr * sizeof (KlAst*));
      newarr[0] = idthis;   /* add a parameter named 'this' */
      funcparams->exprs = newarr;
      ++funcparams->nexpr;
      func->is_method = true;
      klast_setposition(func, begin, expr->end);
      return klast(func);
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

static KlAstExprList* klparser_correctfuncparams(KlParser* parser, KlLex* lex, KlAst* expr, bool* vararg) {
  *vararg = false;
  if (klast_kind(expr) == KLAST_EXPR_LIST) {
    KlAstExprList* params = klcast(KlAstExprList*, expr);
    KlAst** exprs = params->exprs;
    size_t nexpr = params->nexpr;
    for (size_t i = 0; i < nexpr; ++i) {
      if (klast_kind(exprs[i]) == KLAST_EXPR_VARARG) {
        if (kl_unlikely(i != nexpr - 1))
          klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "'...' can only appear at the end of the parameter list");
        *vararg = true;
        continue;
      }
    }
    if (*vararg) {
      klast_delete(params->exprs[params->nexpr - 1]);
      --params->nexpr;
    }
    return params;
  } else if (klast_kind(expr) == KLAST_EXPR_VARARG) {
    *vararg = true;
    KlFileOffset begin = klast_begin(expr);
    KlFileOffset end = klast_end(expr);
    klast_delete(expr);
    return klparser_emptyexprlist(parser, lex, begin, end);
  } else {
    return klparser_singletonexprlist(parser, lex, expr);
  }
}

static KlAstStmtList* klparser_darrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_DARROW), "");
  kllex_next(lex);
  if (kllex_check(lex, KLTK_LPAREN)) {
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    klparser_returnifnull(exprlist);
    KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(exprlist, begin, end);
    klparser_oomifnull(stmtreturn);
    KlAst** stmts = (KlAst**)malloc(sizeof (KlAst**));
    if (kl_unlikely(!stmts)) {
      klast_delete(stmtreturn);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = klast(stmtreturn);
    KlAstStmtList* block = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
    klparser_oomifnull(block);
    return block;
  } else {
    klparser_check(parser, lex, KLTK_LBRACE);
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstStmtList* block = klparser_stmtlist(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACE);
    klparser_returnifnull(block);
    klast_setposition(block, begin, end);
    return block;
  }
}

static KlAstStmtList* klparser_arrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_ARROW), "");
  kllex_next(lex);
  KlAst* expr = klparser_expr(parser, lex);
  klparser_returnifnull(expr);
  KlAstExprList* retvals = klparser_singletonexprlist(parser, lex, expr);
  klparser_returnifnull(retvals);
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(retvals, expr->begin, expr->end);
  klparser_oomifnull(stmtreturn);
  KlAst** stmts = (KlAst**)malloc(sizeof (KlAst**));
  if (kl_unlikely(!stmts)) {
    klast_delete(stmtreturn);
    return klparser_error_oom(parser, lex);
  }
  stmts[0] = klast(stmtreturn);
  KlAstStmtList* block = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
  klparser_oomifnull(block);
  return block;
}

static KlAstCall* klparser_exprcall(KlParser* parser, KlLex* lex, KlAst* callable) {
  bool is_singleton = kllex_hasleadingblank(lex);
  if (is_singleton) {
    KlAst* unit = klparser_exprunit(parser, lex, NULL);
    if (kl_unlikely(!unit)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    KlAstExprList* params = klparser_singletonexprlist(parser, lex, unit);
    if (kl_unlikely(!params)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klast_delete(params);
      return NULL;
    }
    KlAstCall* call = klast_call_create(callable, params, callable->begin, klast_end(unit));
    klparser_oomifnull(call);
    return call;
  } else {
    KlFileOffset begin = kllex_tokbegin(lex);
    klparser_match(parser, lex, KLTK_LPAREN);
    KlAstExprList* params = klparser_exprlist_mayempty(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RPAREN);
    if (kl_unlikely(!params)) {
      if (callable) klast_delete(callable);
      return NULL;
    }
    if (kl_unlikely(!callable)) {
      klast_delete(params);
      return NULL;
    }
    klast_setposition(params, begin, end);
    KlAstCall* call = klast_call_create(callable, params, callable->begin, klast_end(params));
    klparser_oomifnull(call);
    return call;
  }
}

static KlAst* klparser_exprwhere(KlParser* parser, KlLex* lex, KlAst* expr) {
  kllex_next(lex);
  KlAstStmtList* block;
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    block = klparser_stmtlist(parser, lex);
    KlFileOffset end = kllex_tokend(lex);
    klparser_match(parser, lex, KLTK_RBRACE);
    if (kl_unlikely(!block)) {
      klast_delete(expr);
      return NULL;
    }
    klast_setposition(block, begin, end);
  } else {
    KlAst* stmt = klparser_stmt_nosemi(parser, lex);
    if (kl_unlikely(!stmt)) {
      klast_delete(expr);
      return NULL;
    }
    KlAst** stmts = (KlAst**)malloc(1 * sizeof (KlAst*));
    if (kl_unlikely(!stmts)) {
      klast_delete(stmt);
      klast_delete(expr);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    block = klast_stmtlist_create(stmts, 1, klast_begin(stmt), klast_end(stmt));
    if (kl_unlikely(!block)) {
      klast_delete(expr);
      return klparser_error_oom(parser, lex);
    }
  }
  KlAstWhere* exprwhere = klast_where_create(expr, block, klast_begin(expr), klast_end(block));
  klparser_oomifnull(exprwhere);
  return klast(exprwhere);
}

static KlAst* klparser_finishappend(KlParser* parser, KlLex* lex) {
  KArray elemarr;
  if (kl_unlikely(!karray_init(&elemarr)))
    return klparser_error_oom(parser, lex);
  while (kllex_trymatch(lex, KLTK_APPEND)) {
    KlAst* unit = klparser_exprunit(parser, lex, NULL);
    if (kl_unlikely(!unit)) continue;
    klparser_karr_pushast(&elemarr, unit);
  }
  karray_shrink(&elemarr);
  size_t nelem = karray_size(&elemarr);
  if (nelem == 0) {
    karray_destroy(&elemarr);
    return NULL;
  }
  KlAst** elems = (KlAst**)karray_steal(&elemarr);
  KlAstExprList* exprlist = klast_exprlist_create(elems, nelem, klast_begin(elems[0]), klast_end(elems[nelem - 1]));
  return klast(exprlist);
}

static KlAst* klparser_exprpost(KlParser* parser, KlLex* lex) {
  bool inparenthesis;
  KlAst* postexpr = klparser_exprunit(parser, lex, &inparenthesis);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LBRACKET: { /* index */
        if (kllex_hasleadingblank(lex)) {
          postexpr = klast(klparser_exprcall(parser, lex, postexpr));
          break;
        }
        kllex_next(lex);
        KlAst* expr = klparser_expr(parser, lex);
        KlFileOffset end = kllex_tokend(lex);
        klparser_match(parser, lex, KLTK_RBRACKET);
        if (kl_unlikely(!expr)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(expr);
          break;
        }
        KlAstPost* index = klast_post_create(KLTK_INDEX, postexpr, expr, postexpr->begin, end);
        klparser_oomifnull(index);
        postexpr = klast(index);
        break;
      }
      case KLTK_DARROW: {
        postexpr = inparenthesis ? klast(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlAstStmtList* block = klparser_darrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(block);
          break;
        }
        bool vararg;
        KlAstExprList* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klast_delete(block);
          return NULL;
        }
        KlAstFunc* func = klast_func_create(block, params, vararg, false, klast_begin(params), klast_end(block));
        klparser_oomifnull(func);
        postexpr = klast(func);
        break;
      }
      case KLTK_ARROW: {
        postexpr = inparenthesis ? klast(klparser_singletonexprlist(parser, lex, postexpr)) : postexpr;
        KlAstStmtList* block = klparser_arrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) break;
        if (kl_unlikely(!postexpr)) {
          klast_delete(block);
          break;
        }
        bool vararg;
        KlAstExprList* params = klparser_correctfuncparams(parser, lex, postexpr, &vararg);
        if (kl_unlikely(!params)) {
          klast_delete(block);
          return NULL;
        }
        KlAstFunc* func = klast_func_create(block, params, vararg, false, klast_begin(params), klast_end(block));
        klparser_oomifnull(func);
        postexpr = klast(func);
        break;
      }
      case KLTK_DOT: {
        kllex_next(lex);
        if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !postexpr))
          break;
        KlAstDot* dot = klast_dot_create(postexpr, lex->tok.string, postexpr->begin, kllex_tokend(lex));
        klparser_oomifnull(dot);
        postexpr = klast(dot);
        kllex_next(lex);
        break;
      }
      case KLTK_APPEND: {
        KlAst* vals = klparser_finishappend(parser, lex);
        if (kl_unlikely(!postexpr || !vals)) {
          if (vals) klast_delete(vals);
          break;
        }
        KlAstPost* arrpush = klast_post_create(KLTK_APPEND, postexpr, vals, postexpr->begin, vals->end);
        klparser_oomifnull(arrpush);
        postexpr = klast(arrpush);
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
        postexpr = klast(klparser_exprcall(parser, lex, postexpr));
        break;
      }
      default: {  /* no more postfix, just return */
        return postexpr;
      }
    }
  }
}

static KlAst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio) {
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
  KlAst* left = klparser_exprpre(parser, lex);
  KlTokenKind op = kllex_tokkind(lex);
  while (kltoken_isbinop(op) && binop_prio[op] > prio) {
    kllex_next(lex);
    KlAst* right = klparser_exprbin(parser, lex, binop_prio[op]);
    if (kl_unlikely(!left || !right)) {
      if (left) klast_delete(left);
      if (right) klast_delete(right);
      return NULL;
    }
    KlAstBin* binexpr = klast_bin_create(op, left, right, left->begin, right->end);
    klparser_oomifnull(binexpr);
    left = klast(binexpr);
    op = kllex_tokkind(lex);
  }
  return klast(left);
}



KlAstStmtList* klparser_file(KlParser* parser, KlLex* lex) {
  KlAstStmtList* stmtlist = klparser_stmtlist(parser, lex);
  klparser_returnifnull(stmtlist);
  klast_setposition(stmtlist, 0, kllex_tokbegin(lex));
  klparser_match(parser, lex, KLTK_END);
  return stmtlist;
}

static inline KlAst* klparser_stmtbreak(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_BREAK), "expect 'break'");
  KlAstStmtBreak* stmtbreak = klast_stmtbreak_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtbreak)) return klparser_error_oom(parser, lex);
  return klcast(KlAst*, stmtbreak);
}

static inline KlAst* klparser_stmtcontinue(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CONTINUE), "expect 'continue'");
  KlAstStmtContinue* stmtcontinue = klast_stmtcontinue_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtcontinue)) return klparser_error_oom(parser, lex);
  return klcast(KlAst*, stmtcontinue);
}


static KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFileOffset begin = kllex_tokbegin(lex);
    kllex_next(lex);
    KlAstStmtList* stmtlist = klparser_stmtlist(parser, lex);
    if (stmtlist) klast_setposition(stmtlist, begin, kllex_tokend(lex));
    klparser_match(parser, lex, KLTK_RBRACE);
    return stmtlist;
  } else {
    KlAst* stmt = klparser_stmt(parser, lex);
    klparser_returnifnull(stmt);
    KlAst** stmts = (KlAst**)malloc(1 * sizeof (KlAst*));
    if (kl_unlikely(!stmts)) {
      klast_delete(stmt);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, 1, stmt->begin, stmt->end);
    klparser_oomifnull(stmtlist);
    return stmtlist;
  }
}

KlAstStmtList* klparser_singletonstmtlist(KlParser* parser, KlLex* lex) {
  KlAst* stmt = klparser_stmt(parser, lex);
  if (kl_unlikely(!stmt)) return NULL;
  KlAst** stmts = (KlAst**)malloc(sizeof (KlAst*));
  if (kl_unlikely(!stmts)) {
    klast_delete(stmt);
    return NULL;
  }
  stmts[0] = stmt;
  KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, 1, klast_begin(stmt), klast_end(stmt));
  klparser_oomifnull(stmtlist);
  return stmtlist;
}

KlAst* klparser_stmt(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_LET: {
      KlAst* stmtlet = klparser_stmtlet(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtlet;
    }
    case KLTK_IF: {
      KlAstStmtIf* stmtif = klparser_stmtif(parser, lex);
      return klast(stmtif);
    }
    case KLTK_REPEAT: {
      KlAst* stmtrepeat = klparser_stmtrepeat(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtrepeat;
    }
    case KLTK_WHILE: {
      KlAst* stmtwhile = klparser_stmtwhile(parser, lex);
      return stmtwhile;
    }
    case KLTK_RETURN: {
      KlAst* stmtreturn = klparser_stmtreturn(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtreturn;
    }
    case KLTK_BREAK: {
      KlAst* stmtbreak = klparser_stmtbreak(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtbreak;
    }
    case KLTK_CONTINUE: {
      KlAst* stmtcontinue = klparser_stmtcontinue(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return stmtcontinue;
    }
    case KLTK_FOR: {
      KlAst* stmtfor = klparser_stmtfor(parser, lex);
      return stmtfor;
    }
    default: {
      KlAst* res = klparser_stmtexprandassign(parser, lex);
      klparser_match(parser, lex, KLTK_SEMI);
      return res;
    }
  }
}

static KlAst* klparser_stmt_nosemi(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_LET: {
      KlAst* stmtlet = klparser_stmtlet(parser, lex);
      return stmtlet;
    }
    case KLTK_IF: {
      KlAstStmtIf* stmtif = klparser_stmtif(parser, lex);
      return klast(stmtif);
    }
    case KLTK_REPEAT: {
      KlAst* stmtrepeat = klparser_stmtrepeat(parser, lex);
      return stmtrepeat;
    }
    case KLTK_WHILE: {
      KlAst* stmtwhile = klparser_stmtwhile(parser, lex);
      return stmtwhile;
    }
    case KLTK_RETURN: {
      KlAst* stmtreturn = klparser_stmtreturn(parser, lex);
      return stmtreturn;
    }
    case KLTK_BREAK: {
      KlAst* stmtbreak = klparser_stmtbreak(parser, lex);
      return stmtbreak;
    }
    case KLTK_CONTINUE: {
      KlAst* stmtcontinue = klparser_stmtcontinue(parser, lex);
      return stmtcontinue;
    }
    case KLTK_FOR: {
      KlAst* stmtfor = klparser_stmtfor(parser, lex);
      return stmtfor;
    }
    default: {
      KlAst* res = klparser_stmtexprandassign(parser, lex);
      return res;
    }
  }
}
static KlAst* klparser_discoveryforblock(KlParser* parser, KlLex* lex) {
  klparser_match(parser, lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  if (block) klast_delete(block);
  return NULL;
}

static inline KlAstStmtList* klparser_finishstmtfor(KlParser* parser, KlLex* lex) {
  kllex_trymatch(lex, KLTK_COLON);
  return klparser_stmtblock(parser, lex);
}

static KlAst* klparser_stmtinfor(KlParser* parser, KlLex* lex) {
  KlAstExprList* lvals = klparser_exprlist(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* stmtfor -> for i = n, m, s */
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    if (kl_unlikely(!exprlist)) {
      if (lvals) klast_delete(lvals);
      return klparser_discoveryforblock(parser, lex);
    }
    KlAstStmtList* block = klparser_finishstmtfor(parser, lex);
    if (kl_unlikely(!block)) {
      if (lvals) klast_delete(lvals);
      klast_delete(exprlist);
      return NULL;
    }
    if (kl_unlikely(!lvals)) {
      klast_delete(exprlist);
      klast_delete(block);
      return NULL;
    }
    if (kl_unlikely(lvals->nexpr != 1))
      klparser_error(parser, kllex_inputstream(lex), klast_begin(lvals), klast_end(lvals), "integer loop requires only one iteration variable");
    size_t nexpr = exprlist->nexpr;
    if (kl_unlikely(nexpr != 3 && nexpr != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(exprlist), klast_end(exprlist), "expect 2 or 3 expressions here");
      klast_delete(lvals);
      klast_delete(block);
      klast_delete(exprlist);
      return NULL;
    }
    KlAst** exprs = exprlist->exprs;
    KlAstStmtIFor* ifor = klast_stmtifor_create(lvals, exprs[0], exprs[1], nexpr == 3 ? exprs[2] : NULL, block, ph_filepos, klast_end(block));
    klast_exprlist_shallow_replace(exprlist, NULL, 0);
    klast_delete(exprlist);
    klparser_oomifnull(ifor);
    return klast(ifor);
  }
  /* stmtfor -> for a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlAst* iterable = klparser_expr(parser, lex);
  if (kl_unlikely(!iterable)) {
    if (lvals) klast_delete(lvals);
    return klparser_discoveryforblock(parser, lex);
  }
  KlAstStmtList* block = klparser_finishstmtfor(parser, lex);
  if (kl_unlikely(!block)) {
    if (lvals) klast_delete(lvals);
    klast_delete(iterable);
    return NULL;
  }
  if (kl_unlikely(!lvals)) {
    klast_delete(iterable);
    klast_delete(block);
    return NULL;
  }
  kl_assert(lvals->nexpr != 0, "");
  if (klast_kind(iterable) == KLAST_EXPR_VARARG) {
    /* is variable argument for loop : stmtfor -> for a, b, ... in ... */
    klast_delete(iterable); /* 'iterable' is no longer needed */
    KlAstStmtVFor* vfor = klast_stmtvfor_create(lvals, block, ph_filepos, klast_end(block));
    klparser_oomifnull(vfor);
    return klast(vfor);
  } else {  /* generic for loop : stmtfor -> for a, b, ... in expr */
    KlAstStmtGFor* gfor = klast_stmtgfor_create(lvals, iterable, block, ph_filepos, klast_end(block));
    klparser_oomifnull(gfor);
    return klast(gfor);
  }
}

static KlAst* klparser_stmtlet(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LET), "expect 'let'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExprList* lvals = klparser_exprlist(parser, lex);
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlAstExprList* exprlist = klparser_exprlist(parser, lex);
  if (kl_unlikely(!exprlist || !lvals)) {
    if (lvals) klast_delete(lvals);
    if (exprlist) klast_delete(exprlist);
    return NULL;
  }
  KlAstStmtLet* stmtlet = klast_stmtlet_create(lvals, exprlist, begin, klast_end(exprlist));
  klparser_oomifnull(stmtlet);
  return klast(stmtlet);
}

static KlAst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex) {
  KlAstExprList* maybelvals = klparser_exprlist(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* is assignment */
    KlAstExprList* rvals = klparser_exprlist(parser, lex);
    if (kl_unlikely(!rvals || !maybelvals)) {
      if (rvals) klast_delete(rvals);
      if (maybelvals) klast_delete(maybelvals);
      return NULL;
    }
    KlAstStmtAssign* stmtassign = klast_stmtassign_create(maybelvals, rvals, klast_begin(maybelvals), klast_end(rvals));
    klparser_oomifnull(stmtassign);
    return klast(stmtassign);
  } else {
    klparser_returnifnull(maybelvals);
    /* just an expression list statement */
    KlAstStmtExpr* stmtexpr = klast_stmtexpr_create(maybelvals, klast_begin(maybelvals), klast_end(maybelvals));
    klparser_oomifnull(stmtexpr);
    return klast(stmtexpr);
  }
}

static KlAstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_IF), "expect 'if'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* cond = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlAstStmtList* then_block = NULL;
  then_block = klparser_stmtblock(parser, lex);
  KlAstStmtList* else_block = NULL;
  if (kllex_trymatch(lex, KLTK_ELSE)) {     /* else block */
    kllex_trymatch(lex, KLTK_COLON);
    else_block = klparser_stmtblock(parser, lex);
  }
  if (kl_unlikely(!then_block || !cond)) {
    if (cond) klast_delete(cond);
    if (then_block) klast_delete(then_block);
    return NULL;
  }
  KlAstStmtIf* stmtif = klast_stmtif_create(cond, then_block, else_block, begin, else_block ? klast_end(else_block): klast_end(then_block));
  klparser_oomifnull(stmtif);
  return stmtif;
}

static KlAst* klparser_stmtfor(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_FOR), "expect '('");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* stmtfor = klparser_stmtinfor(parser, lex);
  klparser_returnifnull(stmtfor);
  klast_setposition(klcast(KlAst*, stmtfor), begin, stmtfor->end);
  return stmtfor;
}

static KlAst* klparser_stmtwhile(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_WHILE), "expect 'while'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAst* cond = klparser_expr(parser, lex);
  klparser_match(parser, lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  if (kl_unlikely(!cond || !block)) {
    if (cond) klast_delete(cond);
    if (block) klast_delete(block);
    return NULL;
  }
  KlAstStmtWhile* stmtwhile = klast_stmtwhile_create(cond, block, begin, klast_end(block));
  klparser_oomifnull(stmtwhile);
  return klast(stmtwhile);
}

static KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex) {
  KArray stmts;
  if (kl_unlikely(!karray_init(&stmts)))
    return klparser_error_oom(parser, lex);
  while (true) {
    switch (kllex_tokkind(lex)) {
      default: {
        if(!klparser_exprbegin(lex)) break;
        KL_FALLTHROUGH;
      }
      case KLTK_LET: case KLTK_IF: case KLTK_REPEAT: case KLTK_WHILE:
      case KLTK_FOR: case KLTK_RETURN: case KLTK_BREAK: case KLTK_CONTINUE: {
        KlAst* stmt = klparser_stmt(parser, lex);
        if (kl_unlikely(!stmt)) continue;
        if (kl_unlikely(!karray_push_back(&stmts, stmt))) {
          klast_delete(stmt);
          klparser_error_oom(parser, lex);
        }
        continue;
      }
    }
    break;
  }
  karray_shrink(&stmts);
  size_t nstmt = karray_size(&stmts);
  KlAstStmtList* stmtlist = klast_stmtlist_create((KlAst**)karray_steal(&stmts), nstmt, ph_filepos, ph_filepos);
  klparser_oomifnull(stmtlist);
  return stmtlist;
}

static KlAst* klparser_stmtrepeat(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_REPEAT), "expect 'repeat'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  kllex_trymatch(lex, KLTK_COLON);
  KlAstStmtList* block = klparser_stmtblock(parser, lex);
  klparser_match(parser, lex, KLTK_UNTIL);
  KlAst* cond = klparser_expr(parser, lex);
  if (kl_unlikely(!cond || !block)) {
    if (cond) klast_delete(cond);
    if (block) klast_delete(block);
    return NULL;
  }
  KlAstStmtRepeat* stmtrepeat = klast_stmtrepeat_create(block, cond, begin, cond->end);
  klparser_oomifnull(stmtrepeat);
  return klcast(KlAst*, stmtrepeat);
}

static KlAst* klparser_stmtreturn(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_RETURN), "expect 'return'");
  KlFileOffset begin = kllex_tokbegin(lex);
  kllex_next(lex);
  KlAstExprList* results = klparser_exprlist_mayempty(parser, lex);
  klparser_returnifnull(results);
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(results, begin, klast_end(results));
  klparser_oomifnull(stmtreturn);
  return klast(stmtreturn);
}

static KlAst* klparser_generatorfor(KlParser* parser, KlLex* lex, KlAstExprList* lvals, KlAst* inner_stmt) {
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* i = n, m, s */
    KlAstExprList* exprlist = klparser_exprlist(parser, lex);
    kllex_trymatch(lex, KLTK_SEMI);
    if (kl_unlikely(!exprlist)) {
      klast_delete(exprlist);
      klast_delete(lvals);
      klast_delete(inner_stmt);
      return NULL;
    }
    size_t nelem = exprlist->nexpr;
    if (kl_unlikely(nelem != 3 && nelem != 2)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(exprlist), klast_end(exprlist), "expect 2 or 3 expressions here");
      klast_delete(exprlist);
      klast_delete(inner_stmt);
      klast_delete(lvals);
      return NULL;
    }
    if (kl_unlikely(lvals->nexpr != 1)) {
      klparser_error(parser, kllex_inputstream(lex), klast_begin(lvals), klast_end(lvals), "integer loop requires only one iteration variable");
      klast_delete(exprlist);
      klast_delete(inner_stmt);
      klast_delete(lvals);
      return NULL;
    }
    KlAstStmtList* block = klparser_generator(parser, lex, inner_stmt);
    if (kl_unlikely(!block)) {
      klast_delete(exprlist);
      klast_delete(lvals);
      return NULL;
    }
    KlAst** exprs = exprlist->exprs;
    KlAstStmtIFor* ifor = klast_stmtifor_create(lvals, exprs[0], exprs[1], nelem == 3 ? exprs[2] : NULL, block, klast_begin(lvals), klast_end(block));
    klast_exprlist_shallow_replace(exprlist, NULL, 0);
    klast_delete(exprlist);
    klparser_oomifnull(ifor);
    return klast(ifor);
  }
  /* a, b, ... in expr */
  klparser_match(parser, lex, KLTK_IN);
  KlAst* iterable = klparser_expr(parser, lex);
  kllex_trymatch(lex, KLTK_SEMI);
  if (kl_unlikely(!iterable)) {
    klast_delete(inner_stmt);
    klast_delete(lvals);
    return NULL;
  }
  KlAstStmtList* block = klparser_generator(parser, lex, inner_stmt);
  if (kl_unlikely(!block)) {
    klast_delete(lvals);
    klast_delete(iterable);
    return NULL;
  }
  if (klast_kind(iterable) == KLAST_EXPR_VARARG) {  /* a, b, ... in ... */
    klast_delete(iterable); /* 'iterable' is no longer needed */
    KlAstStmtVFor* vfor = klast_stmtvfor_create(lvals, block, klast_begin(lvals), klast_end(block));
    klparser_oomifnull(vfor);
    return klast(vfor);
  } else {  /* a, b, ... in expr */
    KlAstStmtGFor* gfor = klast_stmtgfor_create(lvals, iterable, block, klast_begin(lvals), klast_end(block));
    klparser_oomifnull(gfor);
    return klast(gfor);
  }
}

static KlAstStmtList* klparser_generator(KlParser* parser, KlLex* lex, KlAst* inner_stmt) {
  KArray stmtarr;
  if (kl_unlikely(!karray_init(&stmtarr)))
    return klparser_error_oom(parser, lex);
  while (true) {
    if (klparser_exprbegin(lex)) {
      KlAstExprList* exprlist = klparser_exprlist(parser, lex);
      if (kl_unlikely(!exprlist)) {
        kllex_trymatch(lex, KLTK_SEMI);
        continue;
      }
      if (kllex_check(lex, KLTK_IN) || kllex_check(lex, KLTK_ASSIGN)) {
        KlAst* stmt = klparser_generatorfor(parser, lex, exprlist, inner_stmt);
        if (kl_unlikely(!stmt)) {
          /* 'exprlist' is deleted in klparser_generatorfor() */
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmt))) {
          klast_delete(stmt);
          klparser_destroy_astarray(&stmtarr);
          return klparser_error_oom(parser, lex);
        }
      } else {
        kllex_trymatch(lex, KLTK_SEMI);
        KlAstStmtList* block = klparser_generator(parser, lex, inner_stmt);
        if (kl_unlikely(!block)) {
          klast_delete(exprlist);
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        KlAstStmtIf* stmtif = klast_stmtif_create(klast(exprlist), block, NULL, klast_begin(exprlist), klast_end(block));
        if (kl_unlikely(!stmtif)) {
          klparser_destroy_astarray(&stmtarr);
          return NULL;
        }
        if (kl_unlikely(!karray_push_back(&stmtarr, stmtif))) {
          klast_delete(klast(stmtif));
          klparser_destroy_astarray(&stmtarr);
          return klparser_error_oom(parser, lex);
        }
      }
      kl_assert(karray_size(&stmtarr) != 0, "");
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlAst** stmts = (KlAst**)karray_steal(&stmtarr);
      KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, nstmt, stmts[0]->begin, stmts[nstmt - 1]->end);
      klparser_oomifnull(stmtlist);
      return stmtlist;
    } else if (kllex_check(lex, KLTK_LET)) {
      KlAst* stmtlet = klparser_stmtlet(parser, lex);
      if (kl_unlikely(!stmtlet)) continue;
      if (kl_unlikely(!karray_push_back(&stmtarr, stmtlet))) {
        klast_delete(stmtlet);
        klparser_error_oom(parser, lex);
      }
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      KlFileOffset begin = karray_size(&stmtarr) == 0 ? inner_stmt->begin : klast_begin(karray_access(&stmtarr, 0));
      KlFileOffset end = karray_size(&stmtarr) == 0 ? inner_stmt->end : klast_end(karray_access(&stmtarr, karray_size(&stmtarr) - 1));
      if (kl_unlikely(!karray_push_back(&stmtarr, inner_stmt))) {
        klparser_destroy_astarray(&stmtarr);
        klast_delete(inner_stmt);
        return klparser_error_oom(parser, lex);
      }
      karray_shrink(&stmtarr);
      size_t nstmt = karray_size(&stmtarr);
      KlAst** stmts = (KlAst**)karray_steal(&stmtarr);
      KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, nstmt, begin, end);
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
