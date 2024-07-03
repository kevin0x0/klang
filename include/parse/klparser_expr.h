#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_EXPR_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_EXPR_H_

#include "include/parse/kllex.h"
#include "include/parse/klparser.h"
#include "include/parse/klparser_utils.h"
#include "include/parse/kltokens.h"
#include <stdbool.h>

KlAst* klparser_expr(KlParser* parser, KlLex* lex);
KlAstExprList* klparser_emptyexprlist(KlParser* parser, KlLex* lex, KlFileOffset begin, KlFileOffset end);
static inline KlAstExprList* klparser_exprlist_mayempty(KlParser* parser, KlLex* lex);
KlAstExprList* klparser_finishexprlist(KlParser* parser, KlLex* lex, KlAst* expr);
static inline KlAstExprList* klparser_exprlist(KlParser* parser, KlLex* lex);

extern const bool klparser_isexprbegin[KLTK_NTOKEN];
static inline bool klparser_exprbegin(KlLex* lex);


static inline bool klparser_exprbegin(KlLex* lex) {
  return klparser_isexprbegin[kllex_tokkind(lex)];
}

static inline KlAstExprList* klparser_exprlist(KlParser* parser, KlLex* lex) {
  KlAst* headexpr = klparser_expr(parser, lex);
  return headexpr == NULL ? NULL : klparser_finishexprlist(parser, lex, headexpr);
}

static inline KlAstExprList* klparser_exprlist_mayempty(KlParser* parser, KlLex* lex) {
  return klparser_exprbegin(lex) ? klparser_exprlist(parser, lex) : klparser_emptyexprlist(parser, lex, kllex_tokbegin(lex), kllex_tokbegin(lex));
}

#endif


