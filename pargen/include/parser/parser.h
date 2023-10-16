#ifndef KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H

#include "pargen/include/parser/lexer.h"
#include "kevlr/include/lr.h"
#include "kevlr/include/hashmap/priority_map.h"
#include "utils/include/array/karray.h"
#include "utils/include/hashmap/str_map.h"
#include "utils/include/hashmap/strx_map.h"

#include <stdio.h>

#define KEV_ACTFUNC_FUNCNAME  (0)
#define KEV_ACTFUNC_FUNCDEF   (1)


typedef struct tagKevActionFunc {
  int type;
  char* content;
} KevActionFunc;

typedef struct tagKevConfHandler {
  char* handler_name;
  char* attribute;
} KevConfHandler;

/* pargen parser state */
typedef struct tagKevPParserState {
  KArray* symtables;
  size_t curr_symtbl;
  KevStrXMap* symbols;
  KArray* rules;
  KArray* redact; /* reducing action */
  size_t next_priority;
  size_t next_symbol_id;
  size_t err_count;
  KlrSymbol* start;
  KArray* end_symbols;
  KlrSymbol* default_symbol_nt;
  KlrSymbol* default_symbol_t;
  KlrPrioMap* priorities;
  KArray* confhandlers;
  char* algorithm;
} KevPParserState;

bool kev_pargenparser_init(KevPParserState* parser_state);
void kev_pargenparser_destroy(KevPParserState* parser_state);

void kev_pargenparser_parse(KevPParserState* parser_state, KevPLexer* lex);
bool kev_pargenparser_parse_file(KevPParserState* parser_state, const char* filepath);

#endif
