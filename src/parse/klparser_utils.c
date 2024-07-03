#include "include/parse/klparser_utils.h"

KlStrDesc klparser_newtmpid(KlParser* parser, KlLex* lex) {
  char* newid = klstrtbl_allocstring(parser->strtbl, sizeof (unsigned) * CHAR_BIT);
  if (kl_unlikely(!newid)) {
    klparser_error_oom(parser, lex);
    return (KlStrDesc) { .id = 0, .length = 0 };
  }
  newid[0] = '\0';  /* all temporary identifiers begin with '\0' */
  int len = sprintf(newid + 1, "%u", parser->incid++) + 1;
  size_t strid = klstrtbl_pushstring(parser->strtbl, len);
  return (KlStrDesc) { .id = strid, .length = len };
}

/* tool for clean ast karray when error occurred. */
void klparser_destroy_astarray(KArray* arr) {
  for (size_t i = 0; i < karray_size(arr); ++i) {
    KlAst* ast = (KlAst*)karray_access(arr, i);
    /* ast may be NULL when error occurred */
    if (ast) klast_delete(ast);
  }
  karray_destroy(arr);
}
