#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_STMT_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_STMT_H_

#include "include/ast/klast.h"
#include "include/parse/klparser.h"
#include "include/parse/klparser_error.h"


KlAst* klparser_stmt(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex);
KlAstStmtLet* klparser_stmtlet(KlParser* parser, KlLex* lex);
static inline KlAstStmtBreak* klparser_stmtbreak(KlParser* parser, KlLex* lex);
static inline KlAstStmtContinue* klparser_stmtcontinue(KlParser* parser, KlLex* lex);


static inline KlAstStmtBreak* klparser_stmtbreak(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_BREAK), "expected 'break'");
  KlAstStmtBreak* stmtbreak = klast_stmtbreak_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtbreak)) return klparser_error_oom(parser, lex);
  return stmtbreak;
}

static inline KlAstStmtContinue* klparser_stmtcontinue(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CONTINUE), "expected 'continue'");
  KlAstStmtContinue* stmtcontinue = klast_stmtcontinue_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtcontinue)) return klparser_error_oom(parser, lex);
  return stmtcontinue;
}

#endif
