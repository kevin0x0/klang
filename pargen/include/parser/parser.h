#ifndef KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H
#define KEVCC_PARGEN_INCLUDE_PARSER_PARSER_H

#include "utils/include/array/addr_array.h"
#include "utils/include/hashmap/str_map.h"

/* pargen parser state */
typedef struct tagKevPParserState {
  KevStringMap* env_var;
  KevAddrArray* symbols;
  KevAddrArray* rules;
  size_t next_priority;
} KevPParserState;


#endif
