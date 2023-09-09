#ifndef KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H

#include "pargen/include/parser/lexer.h"
#include "kevlr/include/lr.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/hashmap/str_map.h"
#include "utils/include/hashmap/strx_map.h"

#include <stdio.h>


/* pargen parser state */
typedef struct tagKevPParserState {
  KevStringMap* env_var;
  KevStrXMap* symbols;
  KevAddrArray* rules;
  KevAddrArray* redact; /* reducing action */
  size_t next_priority;
  size_t err_count;
  KevSymbol* default_symbol;
} KevPParserState;

bool kev_pargenparser_init(KevPParserState* parser_state);
void kev_pargenparser_destroy(KevPParserState* parser_state);

void kev_pargenparser_parse(KevPParserState* parser_state, KevPLexer* lex);
void kev_pargenparser_parse_file(KevPParserState* parser_state, const char* filepath);

#endif
