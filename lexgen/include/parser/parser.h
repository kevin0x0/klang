#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H

#include "lexgen/include/parser/lexer.h"
#include "lexgen/include/parser/hashmap/strfa_map.h"
#include "lexgen/include/parser/list/pattern_list.h"
#include "utils/include/hashmap/str_map.h"

/* lexgen parser state */
typedef struct tagKevLParserState {
  KevPatternList list;
  KevStringFaMap nfa_map;
  KevStringMap env_var;
} KevLParserState;

bool kev_lexgenparser_init(KevLParserState* parser_state);
void kev_lexgenparser_destroy(KevLParserState* parser_state);

int kev_lexgenparser_statement_nfa_assign(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state);
int kev_lexgenparser_statement_deftoken(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state);
int kev_lexgenparser_statement_env_var_def(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state);
int kev_lexgenparser_statement_import(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state);
int kev_lexgenparser_parse(const char* filepath, KevLParserState* parser_state);
int kev_lexgenparser_lex_src(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state);

#endif
