#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_STMT_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_STMT_H_

#include "include/ast/klast.h"
#include "include/parse/klparser.h"


KlAstStmt* klparser_stmt(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex);
KlAstStmtList* klparser_emptystmtlist(KlParser* parser, KlLex* lex);
KlAstStmtLet* klparser_stmtlet(KlParser* parser, KlLex* lex);

extern const bool klparser_isstmtbegin[KLTK_NTOKEN];
static inline bool klparser_stmtbegin(KlLex* lex);


static inline bool klparser_stmtbegin(KlLex* lex) {
  return klparser_isstmtbegin[kllex_tokkind(lex)];
}

#endif
