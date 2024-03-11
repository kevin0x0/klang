#include "klang/include/parse/klparser.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/cst/klcst_stmt.h"
#include "klang/include/misc/klutils.h"
#include "klang/include/parse/klcfdarr.h"
#include "klang/include/parse/klidarr.h"
#include "klang/include/parse/kllex.h"
#include "klang/include/parse/klstrtab.h"
#include "klang/include/parse/kltokens.h"
#include "utils/include/array/karray.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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
    error = true;                                       \
  }                                                     \
}


static bool klparser_idarray(KlParser* parser, KlLex* lex, KlIdArray* ids, KlFilePos* pend);
static bool klparser_tupletoidarray(KlParser* parser, KlLex* lex, KlCstExprUnit* tuple, KlIdArray* ids);

KlCst* klparser_generator(KlParser* parser, KlLex* lex);

static KlCst* klparser_arrowfuncbody(KlParser* parser, KlLex* lex);


/* parser for expression */
static KlCst* klparser_emptytuple(KlParser* parser, KlLex* lex);
static KlCst* klparser_finishmap(KlParser* parser, KlLex* lex);
static KlCst* klparser_finishclass(KlParser* parser, KlLex* lex);
static bool klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static bool klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals);
static KlCst* klparser_array(KlParser* parser, KlLex* lex);
static KlCst* klparser_finishtuple(KlParser* parser, KlLex* lex, KlCst* expr);
static KlCst* klparser_dotchain(KlParser* parser, KlLex* lex);
static KlCst* klparser_newexpr(KlParser* parser, KlLex* lex);




static inline KlCst* klparser_tuple(KlParser* parser, KlLex* lex) {
  KlCst* headexpr = klparser_expr(parser, lex);
  return headexpr == NULL ? NULL : klparser_finishtuple(parser, lex, headexpr);
}

static KlCst* klparser_emptytuple(KlParser* parser, KlLex* lex) {
  KlCstExprUnit* tuple = klcst_exprunit_create(KLCST_EXPR_TUPLE);
  if (kl_unlikely(!tuple)) return klparser_error_oom(parser, lex);
  tuple->tuple.elems = NULL;
  tuple->tuple.nelem = 0;
  return klcast(KlCst*, tuple);
}

static KlCst* klparser_singletontuple(KlParser* parser, KlLex* lex, KlCst* expr) {
  KlCstExprUnit* tuple = klcst_exprunit_create(KLCST_EXPR_TUPLE);
  KlCst** elems = (KlCst**)malloc(sizeof (KlCst*));
  if (kl_unlikely(!tuple || !elems)) {
    free(tuple);
    free(elems);
    return klparser_error_oom(parser, lex);
  }
  elems[0] = expr;
  tuple->tuple.elems = elems;
  tuple->tuple.nelem = 1;
  return klcast(KlCst*, tuple);
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

KlCst* klparser_exprunit(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_INT: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      cst->literal.type = KL_INT;
      cst->literal.intval = lex->tok.intval;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_STRING: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      cst->literal.type = KL_STRING;
      cst->literal.string = lex->tok.string;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_BOOLVAL: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      cst->literal.type = KL_BOOL;
      cst->literal.boolval = lex->tok.boolval;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_NIL: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_CONSTANT);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      cst->literal.type = KL_NIL;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_ID: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_ID);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      cst->id = lex->tok.string;
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_VARARG: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_VARARG);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_THIS: {
      KlCstExprUnit* cst = klcst_exprunit_create(KLCST_EXPR_THIS);
      if (kl_unlikely(!cst)) return klparser_error_oom(parser, lex);
      klcst_setposition(klcast(KlCst*, cst), lex->tok.begin, lex->tok.end);
      kllex_next(lex);
      return klcast(KlCst*, cst);
    }
    case KLTK_LBRACKET: {
      return klparser_array(parser, lex);
    }
    case KLTK_LBRACE: {
      kllex_next(lex);
      KlCst* cst = kllex_check(lex, KLTK_COLON) ? klparser_finishmap(parser, lex) : klparser_finishclass(parser, lex);
      klparser_match(parser, lex, KLTK_RBRACE);
      return cst;
    }
    case KLTK_LPAREN: {
      KlFilePos begin = lex->tok.begin;
      kllex_next(lex);
      KlFilePos mayend = lex->tok.end;
      if (kllex_trymatch(lex, KLTK_RPAREN)) { /* empty tuple */
        KlCst* tuple = klparser_emptytuple(parser, lex);
        if (kl_unlikely(!tuple)) return NULL;
        klcst_setposition(klcast(KlCst*, tuple), begin, mayend);
        return klcast(KlCst*, tuple);
      }
      KlCst* tuple = klparser_tuple(parser, lex);
      KlFilePos end = lex->tok.end;
      klparser_match(parser, lex, KLTK_RPAREN);
      if (kl_unlikely(!tuple)) return NULL;
      klcst_setposition(klcast(KlCst*, tuple), begin, end);
      return klcast(KlCst*, tuple);
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
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);  /* skip '[' */
  if (kllex_check(lex, KLTK_RBRACKET)) { /* empty array */
    KlFilePos end = lex->tok.end;
    klparser_match(parser, lex, KLTK_RBRACKET);
    KlCst* emptytuple = klparser_emptytuple(parser, lex);
    if (kl_unlikely(!emptytuple)) return NULL;
    KlCstExprUnit* array = klcst_exprunit_create(KLCST_EXPR_ARR);
    if (kl_unlikely(!array)) {
      free(array);
      klcst_delete(emptytuple);
      return klparser_error_oom(parser, lex);
    }
    klcst_setposition(klcast(KlCst*, array), begin, end);
    klcst_setposition(emptytuple, begin, end);
    array->array.exprs = klcast(KlCst*, emptytuple);
    array->array.stmts = NULL;
    return klcast(KlCst*, array);
  }
  /* else either expression list(may have constructor) */
  KlCst* exprs = klparser_tuple(parser, lex);
  if (kl_unlikely(!exprs)) {
    klparser_discardto(lex, KLTK_RBRACKET);
    return NULL;
  }
  KlCst* stmts = NULL;
  if (kllex_trymatch(lex, KLTK_BAR)) { /* array constructor */
    /* TODO: implement parser for array constructor */
    kl_assert(false, "");
  }
  KlFilePos end = lex->tok.end;
  klparser_match(parser, lex, KLTK_RBRACKET);  /* consume ']' */
  KlCstExprUnit* array = klcst_exprunit_create(KLCST_EXPR_ARR);
  if (kl_unlikely(!array)) {
    klcst_delete(exprs);
    if (stmts) klcst_delete(stmts);
    return klparser_error_oom(parser, lex);
  }
  klcst_setposition(klcast(KlCst*, array), begin, end);
  array->array.exprs = exprs;
  array->array.stmts = stmts;
  return klcast(KlCst*, array);
}

static KlCst* klparser_finishtuple(KlParser* parser, KlLex* lex, KlCst* expr) {
  KArray exprs;
  if (kl_unlikely(!karray_init(&exprs)))
    return klparser_error_oom(parser, lex);
  bool error = false;
  if (kl_unlikely(!karray_push_back(&exprs, expr)))
    error = true;
  while (kllex_trymatch(lex, KLTK_COMMA)) {
    KlCst* expr = klparser_expr(parser, lex);
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
    return klparser_error_oom(parser, lex);
  }
  karray_shrink(&exprs);
  klcst_setposition(klcast(KlCst*, tuple), klcast(KlCst*, karray_front(&exprs))->begin, klcast(KlCst*, karray_top(&exprs))->end);
  tuple->tuple.nelem = karray_size(&exprs);
  tuple->tuple.elems = (KlCst**)karray_steal(&exprs);
  return klcast(KlCst*, tuple);
}

static KlCst* klparser_finishmap(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_COLON), "expect ':'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KArray keys;
  KArray vals;
  if (kl_unlikely(!karray_init(&keys) || !karray_init(&vals))) {
    karray_destroy(&keys);
    karray_destroy(&vals);
    klparser_discarduntil(lex, KLTK_RBRACE); /* assume that map is alway wrapped by "{}" */
    return klparser_error_oom(parser, lex);
  }
  bool error = false;
  if (!kllex_check(lex, KLTK_COLON)) {  /* non-empty */
    /* parse all key-value pairs */
    do {
      KlCst* key = klparser_expr(parser, lex);
      if (kl_unlikely(!klparser_match(parser, lex, KLTK_COLON))) {
        klparser_destroy_cstarray(&keys);
        klparser_destroy_cstarray(&vals);
        klparser_discarduntil(lex, KLTK_RBRACE);
        return NULL;
      }
      KlCst* val = klparser_expr(parser, lex);
      if (kl_unlikely(!key || !val))
        error = true;
      klparser_karr_pushcst(&keys, key);
      klparser_karr_pushcst(&vals, val);
    } while (kllex_trymatch(lex, KLTK_COMMA));
  }
  KlFilePos end = lex->tok.end;
  klparser_match(parser, lex, KLTK_COLON);
  KlCstExprUnit* map = klcst_exprunit_create(KLCST_EXPR_MAP);
  if (kl_unlikely(!map || error)) {
    klparser_destroy_cstarray(&keys);
    klparser_destroy_cstarray(&vals);
    if (!map) klparser_error_oom(parser, lex);
    free(map);
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

static KlCst* klparser_finishclass(KlParser* parser, KlLex* lex) {
  KlFilePos begin = lex->tok.begin;
  KlCfdArray fields;
  KArray vals;
  if (kl_unlikely(!klcfd_init(&fields, 16) || !karray_init(&vals))) {
    klcfd_destroy(&fields);
    karray_destroy(&vals);
    klparser_discarduntil(lex, KLTK_RBRACE);
    return klparser_error_oom(parser, lex);
  }
  bool error = false;
  while (!kllex_check(lex, KLTK_RBRACE)) {
    if (kllex_check(lex, KLTK_LOCAL)) {
      kllex_next(lex);
      if (kl_unlikely(klparser_locallist(parser, lex, &fields, &vals)))
        error = true;
      kllex_trymatch(lex, KLTK_SEMI);
    } else if (kl_likely(!kllex_check(lex, KLTK_END))) {
      kllex_trymatch(lex, KLTK_SHARED);
      if (kl_unlikely(klparser_sharedlist(parser, lex, &fields, &vals)))
        error = true;
      kllex_trymatch(lex, KLTK_SEMI);
    } else {
      klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "end of file");
      break;
    }
  }
  KlFilePos end = lex->tok.end;
  KlCstExprUnit* klclass = klcst_exprunit_create(KLCST_EXPR_CLASS);
  if (kl_unlikely(!klclass || error)) {
    klcfd_destroy(&fields);
    klparser_destroy_cstarray(&vals);
    if (!klclass) klparser_error_oom(parser, lex);
    free(klclass);
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

static bool klparser_locallist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = false;
  bool error = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return true;
    fielddesc.name = lex->tok.string;
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc) || !karray_push_back(vals, NULL))) {
      klparser_error_oom(parser, lex);
      error = true;
    }
    kllex_next(lex);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return error;
}

static bool klparser_sharedlist(KlParser* parser, KlLex* lex, KlCfdArray* fields, KArray* vals) {
  KlCstClassFieldDesc fielddesc;
  fielddesc.shared = true;
  bool error = false;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID)))
      return true;
    fielddesc.name = lex->tok.string;
    kllex_next(lex);
    klparser_match(parser, lex, KLTK_ASSIGN);
    KlCst* val = klparser_expr(parser, lex);
    if (kl_unlikely(!klcfd_push_back(fields, &fielddesc))) {
      klparser_error_oom(parser, lex);
      error = true;
    }
    klparser_karr_pushcst(vals, val);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  return error;
}


static KlCst* klparser_dotchain(KlParser* parser, KlLex* lex) {
  KlCst* dotexpr = klparser_exprunit(parser, lex);
  while (kllex_trymatch(lex, KLTK_DOT)) {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !dotexpr))
      continue;
    KlCstExprPost* dot = klcst_exprpost_create(KLCST_EXPR_DOT);
    if (kl_unlikely(!dot)) {
      klparser_error_oom(parser, lex);
      kllex_next(lex);
      continue;
    }
    dot->dot.operand = dotexpr;
    dot->dot.field = lex->tok.string;
    klcst_setposition(klcast(KlCst*, dot), dotexpr->begin, lex->tok.end);
    dotexpr = klcast(KlCst*, dot);
    kllex_next(lex);
  }
  return dotexpr;
}

static KlCst* klparser_newexpr(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_NEW), "");

  kllex_next(lex);
  KlCst* klclass = kllex_check(lex, KLTK_LPAREN) ? klparser_exprunit(parser, lex) : klparser_dotchain(parser, lex);
  KlCst* params = klparser_exprunit(parser, lex);
  KlCstExprPre* newexpr = klcst_exprpre_create(KLCST_EXPR_NEW);
  if (kl_unlikely(!klclass || !params || !newexpr)) {
    if (klclass) klcst_delete(klclass);
    if (params) klcst_delete(params);
    if (!newexpr) klparser_error_oom(parser, lex);
    free(newexpr);
    return NULL;
  }
  newexpr->operand = klclass;
  newexpr->params = params;
  klcst_setposition(klcast(KlCst*, newexpr), klclass->begin, params->end);
  return klcast(KlCst*, newexpr);
}

KlCst* klparser_exprpre(KlParser* parser, KlLex* lex) {
  switch (kllex_tokkind(lex)) {
    case KLTK_MINUS: {
      KlFilePos begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      if (kl_unlikely(!expr)) return NULL;
      KlCstExprPre* neg = klcst_exprpre_create(KLCST_EXPR_NEG);
      if (kl_unlikely(!neg)) {
        klcst_delete(expr);
        return klparser_error_oom(parser, lex);
      }
      neg->operand = expr;
      klcst_setposition(klcast(KlCst*, neg), begin, expr->end);
      return klcast(KlCst*, neg);
    }
    case KLTK_NOT: {
      KlFilePos begin = lex->tok.begin;
      kllex_next(lex);
      KlCst* expr = klparser_exprpre(parser, lex);
      if (kl_unlikely(!expr)) return NULL;
      KlCstExprPre* neg = klcst_exprpre_create(KLCST_EXPR_NOT);
      if (kl_unlikely(!neg)) {
        klcst_delete(expr);
        return klparser_error_oom(parser, lex);
      }
      neg->operand = expr;
      klcst_setposition(klcast(KlCst*, neg), begin, expr->end);
      return klcast(KlCst*, neg);
    }
    case KLTK_ARROW: {
      KlFilePos begin = lex->tok.begin;
      KlCst* block = klparser_arrowfuncbody(parser, lex);
      if (kl_unlikely(!block)) return NULL;
      KlCstExprPost* func = klcst_exprpost_create(KLCST_EXPR_FUNC);
      if (kl_unlikely(!func)) {
        klcst_delete(block);
        klparser_error_oom(parser, lex);
      }
      func->func.block = block;
      func->func.nparam = 0;
      func->func.params = NULL;
      func->func.vararg = false;
      klcst_setposition(klcast(KlCst*, func), begin, block->end);
      return klcast(KlCst*, func);
    }
    case KLTK_ADD: {
      kllex_next(lex);
      return klparser_exprpre(parser, lex);
    }
    case KLTK_NEW: {
      return klparser_newexpr(parser, lex);
    }
    default: {  /* no prefix */
      return klparser_exprpost(parser, lex);
    }
  }
}

bool klparser_tofuncparams(KlParser* parser, KlLex* lex, KlCst* unit, KlIdArray* params, bool* vararg) {
  *vararg = false;
  if (klcst_kind(unit) == KLCST_EXPR_TUPLE) {
    KlCstExprUnit* tuple = klcast(KlCstExprUnit*, unit);
    KlCst** exprs = tuple->tuple.elems;
    size_t nexpr = tuple->tuple.nelem;
    bool error = false;
    for (size_t i = 0; i < nexpr; ++i) {
      if (klcst_kind(exprs[i]) == KLCST_EXPR_VARARG) {
        *vararg = true;
        if (kl_unlikely(i != nexpr - 1)) {
          klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "'...' can only appear at the end of the parameter list");
          error = true;
        }
        break;
      }
      if (klcst_kind(exprs[i]) != KLCST_EXPR_ID) {
        klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "expect a single identifier, got an expression");
        error = true;
        continue;
      }
      if (kl_unlikely(!klidarr_push_back(params, &klcast(KlCstExprUnit*, exprs[i])->id))) {
        klparser_error_oom(parser, lex);
        error = true;
      }
    }
    klidarr_shrink(params);
    return error;
  } else if (klcst_kind(unit) == KLCST_EXPR_ID) {
    if (kl_unlikely(!klidarr_push_back(params, &klcast(KlCstExprUnit*, unit)->id))) {
      klparser_error_oom(parser, lex);
      return true;
    }
    klidarr_shrink(params);
    return false;
  } else {
    klparser_error(parser, kllex_inputstream(lex), unit->begin, unit->end, "expect parameter list");
    return true;
  }
}

static KlCst* klparser_arrowfuncbody(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_ARROW), "");
  kllex_next(lex);
  KlCst* expr = klparser_expr(parser, lex);
  if (kl_unlikely(!expr)) return NULL;
  KlCstStmtReturn* stmtreturn = klcst_stmtreturn_create();
  if (kl_unlikely(!stmtreturn)) {
    klcst_delete(expr);
    return klparser_error_oom(parser, lex);
  }
  KlCst* retval = klparser_singletontuple(parser, lex, expr);
  if (kl_unlikely(!retval)) {
    klcst_delete(expr);
    free(stmtreturn);
    return NULL;
  }
  stmtreturn->retval = retval;
  klcst_setposition(retval, expr->begin, expr->end);
  klcst_setposition(klcast(KlCst*, stmtreturn), retval->begin, retval->end);
  KlCstStmtList* block = klcst_stmtlist_create();
  KlCst** stmts = (KlCst**)malloc(sizeof (KlCst*));
  if (kl_unlikely(!block || !stmts)) {
    klcst_delete(klcast(KlCst*, stmtreturn));
    free(block);
    free(stmts);
    return klparser_error_oom(parser, lex);
  }
  stmts[0] = klcast(KlCst*, stmtreturn);
  block->stmts = stmts;
  block->nstmt = 1;
  klcst_setposition(klcast(KlCst*, block), retval->begin, retval->end);
  return klcast(KlCst*, block);
}

KlCst* klparser_exprpost(KlParser* parser, KlLex* lex) {
  KlCst* unit = klparser_exprunit(parser, lex);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LBRACKET: { /* index */
        kllex_next(lex);
        KlCst* expr = klparser_expr(parser, lex);
        KlFilePos end = lex->tok.end;
        klparser_match(parser, lex, KLTK_RBRACKET);
        if (kl_unlikely(!expr)) break;
        if (kl_unlikely(!unit)) {
          klcst_delete(expr);
          break;
        }
        KlCstExprPost* index = klcst_exprpost_create(KLCST_EXPR_INDEX);
        if (kl_unlikely(!index)) {
          klcst_delete(expr);
          break;
        }
        index->index.indexable = unit;
        index->index.index = expr;
        klcst_setposition(klcast(KlCst*, index), unit->begin, end);
        unit = klcast(KlCst*, index);
        break;
      }
      case KLTK_LBRACE: {
        KlIdArray params;
        if (kl_unlikely(!klidarr_init(&params, 4))) {
          klparser_error_oom(parser, lex);
          break;
        }
        KlCst* block = klparser_stmtblock(parser, lex);
        if (kl_unlikely(!block)) {
          klidarr_destroy(&params);
          break;
        }
        if (kl_unlikely(!unit)) {
          klidarr_destroy(&params);
          klcst_delete(block);
          break;
        }
        bool vararg;
        klparser_tofuncparams(parser, lex, unit, &params, &vararg);
        KlCstExprPost* func = klcst_exprpost_create(KLCST_EXPR_FUNC);
        if (kl_unlikely(!func)) {
          klidarr_destroy(&params);
          klcst_delete(block);
          klparser_error_oom(parser, lex);
          break;
        }
        func->func.block = block;
        func->func.nparam = klidarr_size(&params);
        func->func.params = klidarr_steal(&params);
        func->func.vararg = vararg;
        klcst_setposition(klcast(KlCst*, func), unit->begin, block->end);
        klcst_delete(unit);
        unit = klcast(KlCst*, func);
        break;
      }
      case KLTK_ARROW: {
        KlIdArray params;
        if (kl_unlikely(!klidarr_init(&params, 4))) {
          klparser_error_oom(parser, lex);
          break;
        }
        KlCst* block = klparser_arrowfuncbody(parser, lex);
        if (kl_unlikely(!block)) {
          klidarr_destroy(&params);
          break;
        }
        if (kl_unlikely(!unit)) {
          klidarr_destroy(&params);
          klcst_delete(block);
          break;
        }
        bool vararg;
        klparser_tofuncparams(parser, lex, unit, &params, &vararg);
        KlCstExprPost* func = klcst_exprpost_create(KLCST_EXPR_FUNC);
        if (kl_unlikely(!func)) {
          klidarr_destroy(&params);
          klcst_delete(block);
          klparser_error_oom(parser, lex);
          break;
        }
        func->func.block = block;
        func->func.nparam = klidarr_size(&params);
        func->func.params = klidarr_steal(&params);
        func->func.vararg = vararg;
        klcst_setposition(klcast(KlCst*, func), unit->begin, block->end);
        klcst_delete(unit);
        unit = klcast(KlCst*, func);
        break;
      }
      case KLTK_DOT: {
        kllex_next(lex);
        if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID) || !unit))
          break;
        KlCstExprPost* dot = klcst_exprpost_create(KLCST_EXPR_DOT);
        if (kl_unlikely(!dot)) {
          klparser_error_oom(parser, lex);
          kllex_next(lex);
          break;
        }
        dot->dot.operand = unit;
        dot->dot.field = lex->tok.string;
        klcst_setposition(klcast(KlCst*, dot), unit->begin, lex->tok.end);
        unit = klcast(KlCst*, dot);
        kllex_next(lex);
        break;
      }
      case KLTK_LPAREN:
      case KLTK_ID:
      case KLTK_STRING:
      case KLTK_INT:
      case KLTK_BOOLVAL:
      case KLTK_THIS:
      case KLTK_VARARG:
      case KLTK_NIL: {  /* call with 'unit' */
        KlCst* param = klparser_exprunit(parser, lex);
        if (kl_unlikely(!param)) break;
        if (kl_unlikely(!unit)) {
          klcst_delete(param);
          break;
        }
        KlCstExprPost* call = klcst_exprpost_create(KLCST_EXPR_CALL);
        if (kl_unlikely(!call)) {
          klcst_delete(param);
          klparser_error_oom(parser, lex);
          break;
        }
        call->call.param = param;
        call->call.callable = unit;
        klcst_setposition(klcast(KlCst*, call), unit->begin, param->end);
        unit = klcast(KlCst*, call);
        break;
      }
      default: {  /* no more postfix */
        return unit;
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
    [KLTK_CONCAT] = 4,
    [KLTK_ADD]    = 5,
    [KLTK_MINUS]  = 5,
    [KLTK_MUL]    = 6,
    [KLTK_DIV]    = 6,
    [KLTK_MOD]    = 6,
  };
  KlCst* left = klparser_exprpre(parser, lex);
  KlTokenKind op = kllex_tokkind(lex);
  if (kltoken_isbinop(op) && binop_prio[op] > prio) {
    kllex_next(lex);
    KlCst* right = klparser_exprbin(parser, lex, binop_prio[op]);
    if (kl_unlikely(!left || !right)) {
      if (left) klcst_delete(left);
      if (right) klcst_delete(right);
      return NULL;
    }
    KlCstExprBin* binexpr = klcst_exprbin_create(op);
    if (kl_unlikely(!binexpr)) {
      klcst_delete(left);
      klcst_delete(right);
      return klparser_error_oom(parser, lex);
    }
    binexpr->loperand = left;
    binexpr->roperand = right;
    klcst_setposition(klcast(KlCst*, binexpr), left->begin, right->end);
    return klcast(KlCst*, binexpr);
  }
  return klcast(KlCst*, left);
}

KlCst* klparser_exprter(KlParser* parser, KlLex* lex) {
  KlCst* cond = klparser_exprbin(parser, lex, 0);
  if (!kllex_trymatch(lex, KLTK_QUESTION))
    return cond;
  KlCst* lexpr = klparser_expr(parser, lex);
  if (kl_unlikely(!klparser_match(parser, lex, KLTK_COLON))) {
    if (lexpr) klcst_delete(lexpr);
    if (cond) klcst_delete(cond);
    return NULL;
  }
  KlCst* rexpr = klparser_expr(parser, lex);
  if (kl_unlikely(!cond || !lexpr || !rexpr)) {
    if (lexpr) klcst_delete(lexpr);
    if (rexpr) klcst_delete(rexpr);
    if (cond) klcst_delete(cond);
    return NULL;
  }
  KlCstExprTer* ternary = klcst_exprter_create();
  if (kl_unlikely(!ternary)) {
    if (lexpr) klcst_delete(lexpr);
    if (rexpr) klcst_delete(rexpr);
    if (cond) klcst_delete(cond);
    return klparser_error_oom(parser, lex);
  }
  ternary->cond = cond;
  ternary->lexpr = lexpr;
  ternary->rexpr = rexpr;
  klcst_setposition(klcast(KlCst*, ternary), cond->begin, rexpr->end);
  return klcast(KlCst*, ternary);
}


/* parser for statement */
static KlCst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex);

KlCst* klparser_stmtblock(KlParser* parser, KlLex* lex) {
  if (kllex_check(lex, KLTK_LBRACE)) {
    KlFilePos begin = lex->tok.begin;
    kllex_next(lex);
    KlCst* stmtlist = klparser_stmtlist(parser, lex);
    if (stmtlist) klcst_setposition(stmtlist, begin, lex->tok.end);
    klparser_match(parser, lex, KLTK_RBRACE);
    return stmtlist;
  } else {
    KlCst* stmt = klparser_stmt(parser, lex);
    if (kl_unlikely(!stmt)) return NULL;
    KlCstStmtList* stmtlist = klcst_stmtlist_create();
    KlCst** stmts = (KlCst**)malloc(1 * sizeof (KlCst*));
    if (kl_unlikely(!stmts || !stmtlist)) {
      free(stmtlist);
      free(stmts);
      klcst_delete(stmt);
      return klparser_error_oom(parser, lex);
    }
    stmts[0] = stmt;
    stmtlist->nstmt = 1;
    stmtlist->stmts = stmts;
    klcst_setposition(klcast(KlCst*, stmtlist), stmt->begin, stmt->end);
    return klcast(KlCst*, stmtlist);
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
      KlCst* stmtif = klparser_stmtif(parser, lex);
      return stmtif;
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

static bool klparser_helper_equaltostr(KlParser* parser, KlCst* cst, const char* str, size_t len) {
  if (klcst_kind(cst) != KLCST_EXPR_CONSTANT)
    return false;
  KlCstExprUnit* constant = klcast(KlCstExprUnit*, cst);
  if (constant->literal.type != KL_STRING)
    return false;
  if (constant->literal.string.length != len)
    return false;
  size_t stringid = constant->literal.string.id;
  return strncmp(str, klstrtab_getstring(parser->strtab, stringid), len) == 0;
}

static KlCst* klparser_discoveryforblock(KlParser* parser, KlLex* lex) {
  klparser_match(parser, lex, KLTK_COLON);
  KlCst* block = klparser_stmtblock(parser, lex);
  if (block) klcst_delete(block);
  return NULL;
}

static inline KlCst* klparser_finishstmtfor(KlParser* parser, KlLex* lex) {
  klparser_match(parser, lex, KLTK_COLON);
  return klparser_stmtblock(parser, lex);
}

static KlCst* klparser_stmtinfor(KlParser* parser, KlLex* lex) {
  KlIdArray varnames;
  bool error = false;
  if (kl_unlikely(!klidarr_init(&varnames, 4)))
    return klparser_error_oom(parser, lex);
  KlFilePos varnames_begin = lex->tok.begin;
  KlFilePos varnames_end;
  error = klparser_idarray(parser, lex, &varnames, &varnames_end) || error;
  if (kllex_trymatch(lex, KLTK_ASSIGN)) { /* stmtfor -> for i = n, m, s */
    KlCst* exprlist = klparser_tuple(parser, lex);
    size_t nelem = klcast(KlCstExprUnit*, exprlist)->tuple.nelem;
    if (kl_unlikely(nelem != 3 && nelem != 2)) {
      klparser_error(parser, kllex_inputstream(lex), exprlist->begin, exprlist->end, "expect 2 or 3 expressions here");
      error = true;
    }
    if (kl_unlikely(error || !exprlist)) {
      /* discovery */
      if (exprlist) klcst_delete(exprlist);
      klidarr_destroy(&varnames);
      return klparser_discoveryforblock(parser, lex);
    }
    if (kl_unlikely(klidarr_size(&varnames) != 1))
      klparser_error(parser, kllex_inputstream(lex), varnames_begin, varnames_end, "integer loop requires only one iteration variable");
    KlStrDesc id = *klidarr_access(&varnames, 0);
    klidarr_destroy(&varnames);
    KlCst* block = klparser_finishstmtfor(parser, lex);
    KlCstStmtIFor* ifor = klcst_stmtifor_create();
    if (kl_unlikely(!ifor || !block)) {
      if (block) klcst_delete(block);
      if (!ifor) klparser_error_oom(parser, lex);
      free(ifor);
      klcst_delete(exprlist);
      return NULL;
    }
    ifor->id = id;
    KlCst** exprs = klcast(KlCstExprUnit*, exprlist)->tuple.elems;
    ifor->begin = exprs[0];
    ifor->end = exprs[1];
    ifor->step = nelem == 3 ? exprs[2] : NULL;
    ifor->block = block;
    /* we just set end here, begin is inessential */
    klcst_setposition(klcast(KlCst*, ifor), block->begin, block->end);
    return klcast(KlCst*, ifor);
  }
  /* stmtfor -> for a, b, ... in expr1, expr2, ... */
  klparser_match(parser, lex, KLTK_IN);
  KlCst* iterable = klparser_expr(parser, lex);
  if (kl_unlikely(!iterable)) {
    /* discovery */
    klidarr_destroy(&varnames);
    return klparser_discoveryforblock(parser, lex);
  }
  KlCst* block = klparser_finishstmtfor(parser, lex);
  if (klcst_kind(iterable) == KLCST_EXPR_VARARG) {
    /* is variable argument for loop : stmtfor -> for a, b, ... in ... */
    klcst_delete(iterable); /* 'iterable' is no longer needed */
    KlCstStmtVFor* vfor = klcst_stmtvfor_create();
    if (kl_unlikely(!vfor || !block)) {
      klidarr_destroy(&varnames);
      if (block) klcst_delete(block);
      if (!vfor) klparser_error_oom(parser, lex);
      free(vfor);
      return NULL;
    }
    klidarr_shrink(&varnames);
    vfor->nid = klidarr_size(&varnames);
    vfor->ids = klidarr_steal(&varnames);
    vfor->block = block;
    /* we just set end here, begin is inessential */
    klcst_setposition(klcast(KlCst*, vfor), block->begin, block->end);
    return klcast(KlCst*, vfor);
  } else {  /* generic for loop : stmtfor -> for a, b, ... = expr1, expr2, ... */
    KlCstStmtGFor* gfor = klcst_stmtgfor_create();
    if (kl_unlikely(!gfor || !block)) {
      klidarr_destroy(&varnames);
      if (block) klcst_delete(block);
      if (!gfor) klparser_error_oom(parser, lex);
      klcst_delete(iterable);
      free(gfor);
      return NULL;
    }
    klidarr_shrink(&varnames);
    gfor->nid = klidarr_size(&varnames);
    gfor->ids = klidarr_steal(&varnames);
    gfor->block = block;
    gfor->expr = iterable;
    /* we just set end here, begin is inessential */
    klcst_setposition(klcast(KlCst*, gfor), block->begin, block->end);
    return klcast(KlCst*, gfor);
  }
}

KlCst* klparser_stmtlet(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LET), "expect 'let'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KlIdArray varnames;
  if (kl_unlikely(!klidarr_init(&varnames, 4))) {
    klparser_discardto(lex, KLTK_SEMI);
    return klparser_error_oom(parser, lex);
  }
  if (kl_unlikely(klparser_idarray(parser, lex, &varnames, NULL))) {
    klparser_discardto(lex, KLTK_SEMI);
    klidarr_destroy(&varnames);
    return klparser_error_oom(parser, lex);
  }
  klparser_match(parser, lex, KLTK_ASSIGN);
  KlCst* exprlist = klparser_tuple(parser, lex);
  if (kl_unlikely(!exprlist)) {
    klidarr_destroy(&varnames);
    return NULL;
  }
  KlCstStmtLet* stmtlet = klcst_stmtlet_create();
  if (kl_unlikely(!stmtlet)) {
    klcst_delete(exprlist);
    klidarr_destroy(&varnames);
    return klparser_error_oom(parser, lex);
  }
  klidarr_shrink(&varnames);
  stmtlet->nlval = klidarr_size(&varnames);
  stmtlet->lvals = (KlStrDesc*)klidarr_steal(&varnames);
  stmtlet->rvals = exprlist;
  klcst_setposition(klcast(KlCst*, stmtlet), begin, exprlist->end);
  return klcast(KlCst*, stmtlet);
}

static KlCst* klparser_stmtexprandassign(KlParser* parser, KlLex* lex) {
  KlCst* maybelvals = klparser_tuple(parser, lex);
  if (kllex_trymatch(lex, KLTK_ASSIGN)) {
    KlCst* rvals = klparser_tuple(parser, lex);
    if (kl_unlikely(!rvals || !maybelvals)) {
      if (rvals) klcst_delete(rvals);
      if (maybelvals) klcst_delete(maybelvals);
      return NULL;
    }
    KlCstStmtAssign* stmtassign = klcst_stmtassign_create();
    if (kl_unlikely(!stmtassign)) {
      klcst_delete(rvals);
      klcst_delete(maybelvals);
      return klparser_error_oom(parser, lex);
    }
    stmtassign->lvals = maybelvals;
    stmtassign->rvals = rvals;
    klcst_setposition(klcast(KlCst*, stmtassign), maybelvals->begin, rvals->end);
    return klcast(KlCst*, stmtassign);
  }
  KlCstStmtExpr* stmtexpr = klcst_stmtexpr_create();
  if (kl_unlikely(!stmtexpr || !maybelvals)) {
    if (maybelvals) klcst_delete(maybelvals);
    if (!stmtexpr) klparser_error_oom(parser, lex);
    free(stmtexpr);
    return NULL;
  }
  stmtexpr->expr = maybelvals;
  klcst_setposition(klcast(KlCst*, stmtexpr), maybelvals->begin, maybelvals->end);
  return klcast(KlCst*, stmtexpr);
}

KlCst* klparser_stmtif(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_IF), "expect 'if'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* cond = klparser_expr(parser, lex);
  KlCst* if_block = NULL;
  klparser_match(parser, lex, KLTK_COLON);
  if_block = klparser_stmtblock(parser, lex);
  KlCst* else_block = NULL;
  if (kllex_trymatch(lex, KLTK_ELSE)) {     /* else block */
    kllex_trymatch(lex, KLTK_COLON);
    else_block = klparser_stmtblock(parser, lex);
  }

  KlCstStmtIf* stmtif = klcst_stmtif_create();
  if (kl_unlikely(!stmtif || !if_block || !cond)) {
    if (cond) klcst_delete(cond);
    if (if_block) klcst_delete(if_block);
    if (else_block) klcst_delete(else_block);
    if (!stmtif) klparser_error_oom(parser, lex);
    free(stmtif);
    return NULL;
  }
  stmtif->cond = cond;
  stmtif->if_block = if_block;
  stmtif->else_block = else_block;
  klcst_setposition(klcast(KlCst*, stmtif), begin, else_block ? else_block->end : if_block->end);
  return klcast(KlCst*, stmtif);
}

KlCst* klparser_stmtcfor(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_LPAREN), "expect '('");
  kllex_next(lex);
  KlCst* stmtlet = NULL;
  if (!kllex_trymatch(lex, KLTK_SEMI)) {
    stmtlet = klparser_stmtlet(parser, lex);
    klparser_match(parser, lex, KLTK_SEMI);
  }
  KlCst* cond = NULL;
  if (!kllex_trymatch(lex, KLTK_SEMI)) {
    cond = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_SEMI);
  }
  KlCst* post = NULL;
  if (!kllex_trymatch(lex, KLTK_RPAREN)) {
    post = klparser_expr(parser, lex);
    klparser_match(parser, lex, KLTK_RPAREN);
  }
  KlCst* block = NULL;
  block = klparser_stmtblock(parser, lex);
  KlCstStmtCFor* cfor = klcst_stmtcfor_create();
  if (kl_unlikely(!cfor || !block)) {
    if (stmtlet) klcst_delete(stmtlet);
    if (cond) klcst_delete(stmtlet);
    if (post) klcst_delete(post);
    if (block) klcst_delete(block);
    if (!cfor) klparser_error_oom(parser, lex);
    free(cfor);
    return NULL;
  }
  cfor->init = stmtlet;
  cfor->cond = cond;
  cfor->post = post;
  cfor->block = block;
  /* caller set position of 'cfor' */
  return klcast(KlCst*, cfor);
}

KlCst* klparser_stmtfor(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_FOR), "expect '('");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  if (kllex_check(lex, KLTK_LPAREN)) {
    KlCst* cfor = klparser_stmtcfor(parser, lex);
    if (kl_unlikely(!cfor)) return NULL;
    klcst_setposition(klcast(KlCst*, cfor), begin, klcast(KlCstStmtCFor*, cfor)->block->end);
    return cfor;
  } else {
    KlCst* stmtfor = klparser_stmtinfor(parser, lex);
    if (kl_unlikely(!stmtfor)) return NULL;
    klcst_setposition(klcast(KlCst*, stmtfor), begin, stmtfor->end);
    return stmtfor;
  }
}

KlCst* klparser_stmtwhile(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_WHILE), "expect 'while'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* cond = klparser_expr(parser, lex);
  KlCst* block = NULL;
  klparser_match(parser, lex, KLTK_COLON);
  block = klparser_stmtblock(parser, lex);
  KlCstStmtWhile* stmtwhile = klcst_stmtwhile_create();
  if (kl_unlikely(!stmtwhile || !cond || !block)) {
    if (cond) klcst_delete(cond);
    if (block) klcst_delete(block);
    if (!stmtwhile) klparser_error_oom(parser, lex);
    free(stmtwhile);
    return NULL;
  }
  stmtwhile->cond = cond;
  stmtwhile->block = block;
  klcst_setposition(klcast(KlCst*, stmtwhile), begin, block->end);
  return klcast(KlCst*, stmtwhile);
}

KlCst* klparser_stmtlist(KlParser* parser, KlLex* lex) {
  KArray stmts;
  if (kl_unlikely(!karray_init(&stmts)))
    return klparser_error_oom(parser, lex);
  while (true) {
    switch (kllex_tokkind(lex)) {
      case KLTK_LET: case KLTK_IF: case KLTK_REPEAT: case KLTK_WHILE: case KLTK_ARROW:
      case KLTK_RETURN: case KLTK_BREAK: case KLTK_CONTINUE: case KLTK_FOR: 
      case KLTK_MINUS: case KLTK_ADD: case KLTK_NOT: case KLTK_NEW: case KLTK_THIS:
      case KLTK_INT: case KLTK_STRING: case KLTK_BOOLVAL: case KLTK_NIL: 
      case KLTK_ID: case KLTK_LBRACKET: case KLTK_LBRACE: case KLTK_LPAREN: {
        KlCst* stmt = klparser_stmt(parser, lex);
        if (kl_unlikely(!stmt)) continue;
        if (kl_unlikely(!karray_push_back(&stmts, stmt)))
          klparser_error_oom(parser, lex);
        continue;
      }
      default: break;
    }
    break;
  }
  KlCstStmtList* stmtlist = klcst_stmtlist_create();
  if (kl_unlikely(!stmtlist)) {
    klparser_destroy_cstarray(&stmts);
    return klparser_error_oom(parser, lex);
  }
  karray_shrink(&stmts);
  stmtlist->nstmt = karray_size(&stmts);
  stmtlist->stmts = (KlCst**)karray_steal(&stmts);
  return klcast(KlCst*, stmtlist);
}

KlCst* klparser_stmtrepeat(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_REPEAT), "expect 'repeat'");
  KlFilePos begin = lex->tok.begin;
  kllex_next(lex);
  KlCst* block = NULL;
  kllex_trymatch(lex, KLTK_COLON);
  block = klparser_stmtblock(parser, lex);
  klparser_match(parser, lex, KLTK_UNTIL);
  KlCst* cond = klparser_expr(parser, lex);
  KlCstStmtWhile* stmtrepeat = klcst_stmtwhile_create();
  if (kl_unlikely(!stmtrepeat || !cond || !block)) {
    if (cond) klcst_delete(cond);
    if (block) klcst_delete(block);
    if (!stmtrepeat) klparser_error_oom(parser, lex);
    free(stmtrepeat);
    return NULL;
  }
  stmtrepeat->block = block;
  stmtrepeat->cond = cond;
  klcst_setposition(klcast(KlCst*, stmtrepeat), begin, cond->end);
  return klcast(KlCst*, stmtrepeat);
}

KlCst* klparser_stmtreturn(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_RETURN), "expect 'return'");
  KlFilePos begin = lex->tok.begin;
  KlFilePos end = lex->tok.end;
  kllex_next(lex);
  KlCstStmtReturn* stmtreturn = klcst_stmtreturn_create();
  if (kl_unlikely(!stmtreturn)) {
    klparser_discarduntil(lex, KLTK_SEMI);
    return klparser_error_oom(parser, lex);
  }
  if (kllex_check(lex, KLTK_SEMI)) { /* no returned value */
    KlCst* tuple = klparser_emptytuple(parser, lex);
    if (kl_unlikely(!tuple)) {
      free(stmtreturn);
      return NULL;
    }
    stmtreturn->retval = tuple;
    klcst_setposition(klcast(KlCst*, tuple), end, end);
    klcst_setposition(klcast(KlCst*, stmtreturn), begin, end);
    return klcast(KlCst*, stmtreturn);
  }
  /* else parse expression list */
  KlCst* exprlist = klparser_tuple(parser, lex);
  if (kl_unlikely(!exprlist)) {
    free(stmtreturn);
    return NULL;
  }
  stmtreturn->retval = exprlist;
  klcst_setposition(klcast(KlCst*, stmtreturn), begin, exprlist->end);
  return klcast(KlCst*, stmtreturn);
}


KlCst* klparser_generator(KlParser* parser, KlLex* lex) {
}



bool klparser_idarray(KlParser* parser, KlLex* lex, KlIdArray* ids, KlFilePos* pend) {
  bool error = false;
  KlFilePos end;
  do {
    if (kl_unlikely(!klparser_check(parser, lex, KLTK_ID))) {
      if (pend) *pend = lex->tok.end;
      return true;
    }
    KlStrDesc id = lex->tok.string;
    end = lex->tok.end;
    if (kl_unlikely(!klidarr_push_back(ids, &id))) {
      klparser_error_oom(parser, lex);
      error = true;
    }
    kllex_next(lex);
  } while (kllex_trymatch(lex, KLTK_COMMA));
  if (pend) *pend = end;
  return error;
}

static bool klparser_tupletoidarray(KlParser* parser, KlLex* lex, KlCstExprUnit* tuple, KlIdArray* ids) {
  kl_assert(klcst_kind(klcast(KlCst*, tuple)) == KLCST_EXPR_TUPLE, "compiler error");
  KlCst** exprs = tuple->tuple.elems;
  size_t nexpr = tuple->tuple.nelem;
  bool error = false;
  for (size_t i = 0; i < nexpr; ++i) {
    if (klcst_kind(exprs[i]) != KLCST_EXPR_ID) {
      klparser_error(parser, kllex_inputstream(lex), exprs[i]->begin, exprs[i]->end, "expect a single identifier, got an expression");
      error = true;
      continue;
    }
    if (kl_unlikely(!klidarr_push_back(ids, &klcast(KlCstExprUnit*, exprs[i])->id))) {
      klparser_error_oom(parser, lex);
      error = true;
    }
  }
  klidarr_shrink(ids);
  return error;
}





/* error handler */
void klparser_helper_locateline(Ki* input, size_t line);
void klparser_helper_showline_withcurl(KlParser* parser, Ki* input, KlFilePos begin, KlFilePos end);

void klparser_error(KlParser* parser, Ki* input, KlFilePos begin, KlFilePos end, const char* format, ...) {
  ++parser->errcount;
  Ko* err = parser->err;
  size_t orioffset = ki_tell(input);
  klparser_helper_locateline(input, begin.line);

  unsigned int col = begin.offset - ki_tell(input) + 1;
  ko_printf(err, "%s:%4u:%4u: ", parser->inputname, begin.line, col);
  va_list vlst;
  va_start(vlst, format);
  ko_vprintf(err, format, vlst);
  va_end(vlst);
  ko_putc(err, '\n');

  for (size_t line = begin.line; line <= end.line; ++line)
    klparser_helper_showline_withcurl(parser, input, begin, end);
  ki_seek(input, orioffset);
  ko_putc(err, '\n');
  ko_flush(err);
}

#define kl_isnl(ch)       ((ch) == '\n' || (ch) == '\r')

void klparser_helper_locateline(Ki* input, size_t line) {
  ki_seek(input, 0);
  size_t currline = 1;
  while (currline < line) {
    int ch = ki_getc(input);
    if (kl_isnl(ch)) {
      if ((ch = ki_getc(input)) != '\r' && ch != KOF)
        ki_ungetc(input);
      ++currline;
    } else if (ch == KOF) {
      ++currline;
      break;
    }
  }
}

void klparser_helper_showline_withcurl(KlParser* parser, Ki* input, KlFilePos begin, KlFilePos end) {
  Ko* err = parser->err;
  size_t curroffset = ki_tell(input);
  int ch = ki_getc(input);
  while (!kl_isnl(ch) && ch != KOF) {
    ko_putc(err, ch);
    ch = ki_getc(input);
  }
  ko_putc(err, '\n');
  ki_seek(input, curroffset);
  ch = ki_getc(input);
  while (!kl_isnl(ch) && ch != KOF) {
    if (curroffset >= begin.offset && curroffset < end.offset) {
      if (ch == '\t') {
        for (size_t i = 0; i < parser->config.tabstop; ++i)
          ko_putc(err, parser->config.curl);
      } else {
        ko_putc(err, parser->config.curl);
      }
    } else {
      ch == '\t' ? ko_putc(err, '\t') : ko_putc(err, ' ');
    }
    ch = ki_getc(input);
    ++curroffset;
  }
  ko_putc(err, '\n');
  if (ch != KOF && (ch = ki_getc(input)) != '\r' && ch != KOF)
    ki_ungetc(input);
}
