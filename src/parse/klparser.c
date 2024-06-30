#include "include/parse/klparser.h"
#include "include/parse/kllex.h"
#include "include/parse/klparser_expr.h"
#include "include/parse/klparser_stmt.h"
#include "include/parse/klparser_utils.h"

bool klparser_init(KlParser* parser, KlStrTbl* strtbl, const char* inputname, KlError* klerror) {
  parser->strtbl = strtbl;
  parser->inputname = inputname;
  parser->incid = 0;
  parser->klerror = klerror;
  return true;
}

KlAstStmtList* klparser_file(KlParser* parser, KlLex* lex) {
  KlAstStmtList* stmtlist = klparser_stmtlist(parser, lex);
  klparser_returnifnull(stmtlist);
  klast_setposition(stmtlist, 0, kllex_tokbegin(lex));
  klparser_match(parser, lex, KLTK_END);
  return stmtlist;
}

KlAstStmtList* klparser_interactive(KlParser* parser, KlLex* lex) {
  KlAst* stmt = NULL;
  if (kllex_check(lex, KLTK_END)) {
    KlAstExprList* exprlist = klparser_emptyexprlist(parser, lex, 0, 0);
    klparser_oomifnull(exprlist);
    KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(klcast(KlAstExprList*, exprlist), klast_begin(exprlist), klast_end(exprlist));
    klparser_oomifnull(stmtreturn);
    stmt = klast(stmtreturn);
  } else {
    stmt = klparser_stmt(parser, lex);
    klparser_check(parser, lex, KLTK_SEMI);
    if (kl_unlikely(!stmt)) return NULL;
    if (klast_kind(stmt) == KLAST_STMT_EXPR) {
      KlAstExprList* exprlist = klast_stmtexpr_steal_exprlist_and_destroy(klcast(KlAstStmtExpr*, stmt));
      KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(klcast(KlAstExprList*, exprlist), klast_begin(exprlist), klast_end(exprlist));
      klparser_oomifnull(stmtreturn);
      stmt = klast(stmtreturn);
    }
  }
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

KlAstStmtList* klparser_evaluate(KlParser* parser, KlLex* lex) {
  KlAst* stmt = klparser_stmt(parser, lex);
  klparser_returnifnull(stmt);
  if (klast_kind(stmt) != KLAST_STMT_EXPR) {
    klparser_error(parser, kllex_inputstream(lex), klast_begin(stmt), klast_end(stmt), "expected expression list");
    klast_delete(stmt);
    return NULL;
  }
  KlAstExprList* exprlist = klast_stmtexpr_steal_exprlist_and_destroy(klcast(KlAstStmtExpr*, stmt));
  KlAstStmtReturn* stmtreturn = klast_stmtreturn_create(klcast(KlAstExprList*, exprlist), klast_begin(exprlist), klast_end(exprlist));
  klparser_oomifnull(stmtreturn);
  KlAst** stmts = (KlAst**)malloc(sizeof (KlAst*));
  if (kl_unlikely(!stmts)) {
    klast_delete(stmtreturn);
    return NULL;
  }
  stmts[0] = klast(stmtreturn);
  KlAstStmtList* stmtlist = klast_stmtlist_create(stmts, 1, klast_begin(stmtreturn), klast_end(stmtreturn));
  klparser_oomifnull(stmtlist);
  return stmtlist;

}
