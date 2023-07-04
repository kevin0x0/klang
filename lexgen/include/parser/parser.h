#ifndef KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_LEXGEN_INCLUDE_PARSER_PARSER_H
#include "lexgen/include/parser/lexer.h"
#include "lexgen/include/parser/hashmap/strfa_map.h"
#include "lexgen/include/parser/list/pattern_list.h"

void kev_lexgenparser_statement_assign(KevLexGenLexer* lex, KevLexGenToken* token, KevPatternList* list, KevStringFaMap* nfa_map);
void kev_lexgenparser_statement_deftoken(KevLexGenLexer* lex, KevLexGenToken* token, KevPatternList* list, KevStringFaMap* nfa_map);
void kev_lexgenparser_lex_src(KevLexGenLexer* lex, KevLexGenToken* token, KevPatternList* list, KevStringFaMap* nfa_map);

#endif
