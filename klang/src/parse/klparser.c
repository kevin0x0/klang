#include "klang/include/parse/klparser.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/parse/klcfdarr.h"
#include "klang/include/parse/kllex.h"
#include "klang/include/parse/kltokens.h"
#include "utils/include/array/karray.h"
#include "utils/include/array/kgarray.h"
#include <stdarg.h>
#include <stdlib.h>

KlCst* klparser_error_oom(KlParser* parser);

bool klparser_discarduntil(KlParser* parser, KlTokenKind kind) {
  KlLex* lex = parser->lex;
  while (lex->tok.kind != kind) {
    if (lex->tok.kind == KLTK_END) return false;
    kllex_next(lex);
  }
  return true;
}

bool klparser_discardto(KlParser* parser, KlTokenKind kind) {
  bool success = klparser_discarduntil(parser, kind);
  if (success) kllex_next(parser->lex);
  return success;
}


#define klparser_karr_pushcst(arr, cst)  {              \
  if (kl_unlikely(!karray_push_back(arr, cst))) {       \
    klparser_error_oom(parser);                         \
    if (cst) klcst_delete(cst);                         \
    error = true;                                       \
  }                                                     \
}


KlCst* klparser_finishmap(KlParser* parser);
KlCst* klparser_finishclass(KlParser* parser);
bool klparser_locallist(KlParser* parser, KlCfdArray* fields, KArray* vals);
bool klparser_sharedlist(KlParser* parser, KlCfdArray* fields, KArray* vals);
KlCst* klparser_array(KlParser* parser);
KlCst* klparser_trytuple(KlParser* parser);
KlCst* klparser_finishtuple(KlParser* parser, KlCst* expr);




static inline KlCst* klparser_tuple(KlParser* parser) {
  return klparser_finishtuple(parser, NULL);
}


/* tool for clean cst karray when error occurred. */
void klparser_destroy_cstarray(KArray* arr) {
  for (size_t i = 0; i < karray_size(arr); ++i) {
    KlCst* cst = (KlCst*)karray_access(arr, i);
    /* cst may be NULL when error occurred */
    if (cst) klcst_delete(cst);
  }
  karray_destroy(arr);
}

KlCst* klparser_exprunit(KlParser* parser) {
  KlLex* lex = parser->lex;
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser);
      cst->literal.type = KL_INT;
      cst->literal.intval = lex->tok.intval;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      return klcast(KlCst*, cst);
    }
    case KLTK_STRING: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser);
      cst->literal.type = KL_STRING;
      cst->literal.string = lex->tok.string;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      return klcast(KlCst*, cst);
    }
    case KLTK_BOOLVAL: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser);
      cst->literal.type = KL_BOOL;
      cst->literal.boolval = lex->tok.boolval;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      return klcast(KlCst*, cst);
    }
    case KLTK_ID: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_ID);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser);
      cst->id = lex->tok.string;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      return klcast(KlCst*, cst);
    }
    case KLTK_LBRACKET: {
      return klparser_array(parser);
    }
    case KLTK_LBRACE: {
      kllex_next(lex);
      KlCst* cst = kllex_check(lex, KLTK_COLON) ? klparser_finishmap(parser) : klparser_finishclass(parser);
      klparser_match(parser, KLTK_RBRACE);
      return cst;
    }
    case KLTK_LPAREN: {
      kllex_next(lex);
      KlCst* tuple = klparser_tuple(parser);
      klparser_match(parser, KLTK_RPAREN);
      return klcast(KlCst*, tuple);
    }
    default: {
      klparser_error(parser, "expected '(', '{', '[', true, identifier, false, string or integer");
      return NULL;
    }
  }
}

KlCst* klparser_array(KlParser* parser) {
  KlLex* lex = parser->lex;
  kl_assert(kllex_check(lex, KLTK_LBRACKET), "expected \'[\'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);  /* skip '[' */
  if (kllex_check(lex, KLTK_RBRACKET)) { /* empty array */
    KlCst* emptytuple = klparser_tuple(parser);
    klparser_match(parser, KLTK_RBRACKET);
    KlCstExprUnit* array = klcst_exprunit_create(KLCST_EXPR_ARR);
    if (kl_unlikely(!array)) {
      klcst_delete(emptytuple);
      return klparser_error_oom(parser);
    }
    klcst_setposition(klcast(KlCst*, array), begin, lex->tok.end);
    array->array.arrgen = emptytuple;
    return klcast(KlCst*, array);
  }
  /* else either expression list(tuple) or array constructor */
  KlCst* headexpr = klparser_expr(parser);  /* read first expression */
  if (kl_unlikely(!headexpr)) {
    klparser_discardto(parser, KLTK_RBRACKET);
    return klparser_error_oom(parser);
  }

  KlCst* arrgen = NULL;
  if (kllex_check(lex, KLTK_BAR)) { /* array constructor */
    /* TODO: implement parser for array constructor */
    kl_assert(false, "");
  } else {  /* else is expression list */
    if (kl_unlikely((arrgen = klparser_finishtuple(parser, headexpr)))) {
      klparser_discardto(parser, KLTK_RBRACKET);
      return klparser_error_oom(parser);
    }
  }
  klparser_match(parser, KLTK_RBRACKET);  /* consume ']' */
  KlCstExprUnit* array = klcst_exprunit_create(KLCST_EXPR_ARR);
  if (kl_unlikely(!array)) {
    klcst_delete(arrgen);
    return klparser_error_oom(parser);
  }
  klcst_setposition(klcast(KlCst*, array), begin, lex->tok.end);
  array->array.arrgen = arrgen;
  return klcast(KlCst*, array);
}

KlCst* klparser_finishtuple(KlParser* parser, KlCst* expr) {
  KArray exprs;
  if (kl_unlikely(!karray_init(&exprs)))
    return klparser_error_oom(parser);
  bool error = false;
  if (kl_unlikely(expr && !karray_push_back(&exprs, expr)))
    error = true;
  while (klparser_trymatch(parser, KLTK_COMMA)) {
    KlCst* expr = klparser_expr(parser);
    if (kl_unlikely(!expr)) error = true;
    klparser_karr_pushcst(&exprs, expr);
  }
  if (kl_unlikely(error)) {
    klparser_destroy_cstarray(&exprs);
    return NULL;
  }
  KlCstExprUnit* tuple = klcst_exprunit_create(KLCST_EXPR_TUPLE);
  if (kl_unlikely(!tuple)) {
    klparser_destroy_cstarray(&exprs);
    return klparser_error_oom(parser);
  }
  karray_shrink(&exprs);
  klcst_setposition(klcast(KlCst*, tuple), expr->begin, klcast(KlCst*, karray_top(&exprs))->end);
  tuple->tuple.nelem = karray_size(&exprs);
  tuple->tuple.elems = (KlCst**)karray_steal(&exprs);
  return klcast(KlCst*, tuple);
}

KlCst* klparser_finishmap(KlParser* parser) {
  KlLex* lex = parser->lex;
  kl_assert(kllex_check(lex, KLTK_COLON), "expected :");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KArray keys;
  KArray vals;
  if (kl_unlikely(!karray_init(&keys) || !karray_init(&vals))) {
    karray_destroy(&keys);
    karray_destroy(&vals);
    klparser_discarduntil(parser, KLTK_RBRACE); /* assume that map is alway wrapped by "{}" */
    return klparser_error_oom(parser);
  }
  bool error = false;
  /* parse all key-value pairs */
  while (kllex_check(lex, KLTK_COMMA)) {
    KlCst* key = klparser_expr(parser);
    if (kl_unlikely(!klparser_match(parser, KLTK_COLON))) {
      klparser_destroy_cstarray(&keys);
      klparser_destroy_cstarray(&vals);
      return NULL;
    }
    KlCst* val = klparser_expr(parser);
    if (kl_unlikely(!key || !val))
      error = true;
    klparser_karr_pushcst(&keys, key);
    klparser_karr_pushcst(&vals, val);
  }
  KlFilePos end = lex->tok.end;
  klparser_match(parser, KLTK_COLON);
  KlCstExprUnit* map = klcst_exprunit_create(KLCST_EXPR_MAP);
  if (kl_unlikely(!map || error)) {
    klparser_destroy_cstarray(&keys);
    klparser_destroy_cstarray(&vals);
    if (map) {
      free(map);
      return klparser_error_oom(parser);
    }
    return NULL;
  }
  kl_assert(karray_size(&keys) == karray_size(&vals), "");
  karray_shrink(&keys);
  karray_shrink(&vals);
  map->map.npair = karray_size(&keys);
  map->map.keys = (KlCst**)karray_steal(&keys);
  map->map.vals = (KlCst**)karray_steal(&vals);
  klcst_setposition(klcast(KlCst*, map), begin, end);
  return klcast(KlCst*, map);
}

KlCst* klparser_finishclass(KlParser* parser) {
  KlLex* lex = parser->lex;
  KlFilePos begin = lex->tok.begin;
  KlCfdArray fields;
  KArray vals;
  if (kl_unlikely(!klcfd_init(&fields, 16) || !karray_init(&vals))) {
    klcfd_destroy(&fields);
    karray_destroy(&vals);
    klparser_discarduntil(parser, KLTK_RBRACE);
    return klparser_error_oom(parser);
  }
  bool error = false;
  while (!kllex_check(lex, KLTK_RBRACE)) {
    if (kllex_check(lex, KLTK_LOCAL)) {
      kllex_next(lex);
      if (kl_unlikely(!klparser_locallist(parser, &fields, &vals)))
        error = true;
      klparser_trymatch(parser, KLTK_SEMI);
    } else if (kl_likely(!kllex_check(lex, KLTK_END))) {
      klparser_trymatch(parser, KLTK_SHARED);
      if (kl_unlikely(!klparser_sharedlist(parser, &fields, &vals)))
        error = true;
      klparser_trymatch(parser, KLTK_SEMI);
    } else {
      klparser_error(parser, "end of file");
      break;
    }
  }
  KlFilePos end = lex->tok.end;
  kllex_next(lex);
  KlCstExprUnit* klclass = klcst_exprunit_create(KLCST_EXPR_CLASS);
  if (kl_unlikely(!klclass || error)) {
    klcfd_destroy(&fields);
    klparser_destroy_cstarray(&vals);
    if (klclass) {
      free(klclass);
      return klparser_error_oom(parser);
    }
    return NULL;
  }
  kl_assert(klcfd_size(&fields) == karray_size(&vals), "");
  klcfd_shrink(&fields);
  karray_shrink(&vals);
  klclass->klclass.nfield = karray_size(&vals);
  klclass->klclass.fields = (KlCstClassFieldDesc*)klcfd_steal(&fields);
  klclass->klclass.vals = (KlCst**)karray_steal(&vals);
  klcst_setposition(klcast(KlCst*, klclass), begin, end);
  return klcast(KlCst*, klclass);
}

bool klparser_locallist(KlParser* parser, KlCfdArray* fields, KArray* vals) {
  KlLex* lex = parser->lex;
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = false;
  bool error = false;
  do {
    if (kl_unlikely(!klparser_check(parser, KLTK_ID))) {
      klparser_discarduntil(parser, KLTK_RBRACE);
      return true;
    }
    fielddesc.name = lex->tok.string;
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc) || !karray_push_back(vals, NULL)))
      error = true;
    kllex_next(lex);
  } while (klparser_trymatch(parser, KLTK_COMMA));
  return error;
}

bool klparser_sharedlist(KlParser* parser, KlCfdArray* fields, KArray* vals) {
  KlLex* lex = parser->lex;
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  bool error = false;
  do {
    if (kl_unlikely(!klparser_check(parser, KLTK_ID))) {
      klparser_discarduntil(parser, KLTK_RBRACE);
      return true;
    }
    fielddesc.name = lex->tok.string;
    klparser_match(parser, KLTK_ASSIGN);
    KlCst* val = klparser_expr(parser);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc))) {
      klparser_error_oom(parser);
      error = true;
    }
    klparser_karr_pushcst(vals, val);
    kllex_next(lex);
  } while (klparser_trymatch(parser, KLTK_COMMA));
  return error;
}


KlCst* klparser_exprpre(KlParser* parser) {
  KlLex* lex = parser->lex;
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFilePos begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser);
      if (kl_unlikely(expr)) return NULL;
      KlCstExprPre* neg = klcst_exprpre_create(KLCST_EXPR_NEG);
      if (kl_unlikely(!neg)) {
        klcst_delete(expr);
        return klparser_error_oom(parser);
      }
      neg->oprand = expr;
      klcst_setposition(klcast(KlCst*, neg), begin, expr->end);
      return klcast(KlCst*, neg);
    }
    case KLTK_NOT: {
      KlFilePos begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser);
      if (kl_unlikely(expr)) return NULL;
      KlCstExprPre* neg = klcst_exprpre_create(KLCST_EXPR_NOT);
      if (kl_unlikely(!neg)) {
        klcst_delete(expr);
        return klparser_error_oom(parser);
      }
      neg->oprand = expr;
      klcst_setposition(klcast(KlCst*, neg), begin, expr->end);
      return klcast(KlCst*, neg);
    }
    case KLTK_ADD: {
      kllex_next(lex);
      return klparser_exprpre(parser);
    }
    default: {  /* no prefix */
      return klparser_exprpost(parser);
    }
  }
}

KlCst* klparser_exprpost(KlParser* parser) {
  KlCst* unit = klparser_exprunit(parser);
  KlLex* lex = parser->lex;
  switch (kllex_tokkind(lex)) {
    case KLTK_LBRACKET:
    case KLTK_LBRACE:
    case KLTK_ARROW:
    case KLTK_DOT:
    default:
  }
}

KlCst* klparser_expr(KlParser* parser) {
  return NULL;
}


/* error handler */
void klparser_error(KlParser* parser, const char* format, ...) {
  va_list vlst;
  va_start(vlst, format);
  ko_vprintf(parser->err, format, vlst);
  va_end(vlst);
  ko_putc(parser->err, '\n');
  ko_flush(parser->err);
}

KlCst* klparser_error_oom(KlParser* parser) {
  klparser_error(parser, "out of memory");
  return NULL;
}

