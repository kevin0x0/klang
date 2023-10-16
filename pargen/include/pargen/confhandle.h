#ifndef KEVCC_PARGEN_INCLUDE_PARGEN_CONFHANDLE_H
#define KEVCC_PARGEN_INCLUDE_PARGEN_CONFHANDLE_H

#include "pargen/include/parser/parser.h"
#include "kevlr/include/conflict_handle.h"
#include "kevlr/include/hashmap/priority_map.h"

KlrConflictHandler* kev_pargen_confhandle_get_handler(KevPParserState* parser_state);
void kev_pargen_confhandle_delete(KlrConflictHandler* handler);

#endif
