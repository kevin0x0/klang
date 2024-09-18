#include "include/ast/klast.h"

bool klast_isboolexpr(KlAstExpr* ast) {
  switch (klast_kind(ast)) {
    case KLAST_EXPR_PRE: {
      return klcast(KlAstPre*, ast)->op == KLTK_NOT;
    }
    case KLAST_EXPR_BIN: {
      return klcast(KlAstBin*, ast)->op == KLTK_AND ||
             klcast(KlAstBin*, ast)->op == KLTK_OR  ||
             kltoken_isrelation(klcast(KlAstBin*, ast)->op);
    }
    case KLAST_EXPR_WHERE: {
      return klast_isboolexpr(klcast(KlAstWhere*, ast)->expr);
    }
    default: {
      return false;
    }
  }
}
