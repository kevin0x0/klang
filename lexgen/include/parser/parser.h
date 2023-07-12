#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H

#include "lexgen/include/parser/lexer.h"
#include "lexgen/include/parser/hashmap/strfa_map.h"
#include "lexgen/include/parser/hashmap/str_map.h"
#include "lexgen/include/parser/list/pattern_list.h"

typedef struct tagKevParserState {
  KevPatternList list;
  KevStringFaMap nfa_map;
  KevStringMap env_var;
} KevParserState;

int kev_lexgenparser_statement_assign(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state);
int kev_lexgenparser_statement_deftoken(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state);
int kev_lexgenparser_statement_tmpl_def(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state);
int kev_lexgenparser_lex_src(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state);
bool kev_lexgenparser_posix_charset(KevParserState* parser_state);
bool kev_lexgenparser_init(KevParserState* parser_state);
void kev_lexgenparser_destroy(KevParserState* parser_state);

#endif
