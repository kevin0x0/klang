#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_H_

#include "include/ast/klast.h"
#include "include/ast/klast.h"
#include "include/parse/kllex.h"

typedef struct tagKlParser {
  const char* inputname;
  KlStrTbl* strtbl;
  KlError* klerror;
  struct {
    KlStrDesc this;
  } string;
  unsigned incid;
} KlParser;


bool klparser_init(KlParser* parser, KlStrTbl* strtbl, const char* inputname, KlError* klerror);



KlAst* klparser_expr(KlParser* parser, KlLex* lex);
KlAst* klparser_stmt(KlParser* parser, KlLex* lex);
KlAstStmtList* klparser_file(KlParser* parser, KlLex* lex);




#endif
