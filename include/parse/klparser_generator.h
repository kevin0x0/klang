#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_GENERATOR_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_GENERATOR_H_
#include "include/ast/klast.h"
#include "include/parse/klparser.h"

KlAstStmtList* klparser_generator(KlParser* parser, KlLex* lex, KlAst* inner_stmt);

#endif



