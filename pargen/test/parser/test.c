#include "kevlr/include/collection.h"
#include "kevlr/include/lr.h"
#include "kevlr/include/conflict_handle.h"
#include "kevlr/include/print.h"
#include "kevlr/include/table.h"
#include "pargen/include/parser/lexer.h"
#include "pargen/include/parser/parser.h"


int main(int argc, char** argv) {
  KevPParserState pstate;
  KevPLexer lex;
  FILE* infile = fopen("test.klr", "r");
  kev_pargenlexer_init(&lex, infile);
  kev_pargenparser_init(&pstate);
  kev_pargenlexer_next(&lex);
  kev_pargenparser_parse(&pstate, &lex);
  KevLRCollection* collec = kev_lr_collection_create_lalr(pstate.start, (KevSymbol**)pstate.end_symbols->begin, kev_addrarray_size(pstate.end_symbols));
  KevLRConflictHandler* handler = kev_lr_conflict_handler_create(pstate.priorities, kev_lr_confhandler_priority_callback);
  KevLRTable* table = kev_lr_table_create(collec, handler);
  kev_lr_conflict_handler_delete(handler);
  //kev_lr_print_action_table(stdout, table);
  //kev_lr_print_goto_table(stdout, table);
  //kev_lr_print_symbols(stdout, collec);
  kev_lr_table_delete(table);
  kev_lr_collection_delete(collec);
  kev_pargenparser_destroy(&pstate);
  kev_pargenlexer_destroy(&lex);
  return 0;
}
