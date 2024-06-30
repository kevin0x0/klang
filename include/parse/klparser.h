#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_H_

#include "include/ast/klast.h"
#include "include/ast/klast.h"
#include "include/parse/kllex.h"

typedef struct tagKlParser {
  const char* inputname;
  KlStrTbl* strtbl;
  KlError* klerror;
  unsigned incid;
} KlParser;


bool klparser_init(KlParser* parser, KlStrTbl* strtbl, const char* inputname, KlError* klerror);

KlAstStmtList* klparser_file(KlParser* parser, KlLex* lex);
KlAstStmtList* klparser_interactive(KlParser* parser, KlLex* lex);
KlAstStmtList* klparser_evaluate(KlParser* parser, KlLex* lex);

#endif
