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


const bool klparser_isexprbegin[KLTK_NTOKEN] = {
  [KLTK_ARROW] = true,
  [KLTK_DARROW] = true,
  [KLTK_VARARG] = true,
  [KLTK_LEN] = true,
  [KLTK_MINUS] = true,
  [KLTK_ADD] = true,
  [KLTK_NOT] = true,
  [KLTK_NEW] = true,
  [KLTK_INT] = true,
  [KLTK_INTDOT] = true,
  [KLTK_FLOAT] = true,
  [KLTK_STRING] = true,
  [KLTK_BOOLVAL] = true,
  [KLTK_NIL] = true,
  [KLTK_ID] = true,
  [KLTK_LBRACKET] = true,
  [KLTK_LBRACE] = true,
  [KLTK_YIELD] = true,
  [KLTK_ASYNC] = true,
  [KLTK_CASE] = true,
  [KLTK_INHERIT] = true,
  [KLTK_LPAREN] = true,
};

const bool klparser_isstmtbegin[KLTK_NTOKEN] = {
  [KLTK_ARROW] = true,
  [KLTK_DARROW] = true,
  [KLTK_VARARG] = true,
  [KLTK_LEN] = true,
  [KLTK_MINUS] = true,
  [KLTK_ADD] = true,
  [KLTK_NOT] = true,
  [KLTK_NEW] = true,
  [KLTK_INT] = true,
  [KLTK_INTDOT] = true,
  [KLTK_FLOAT] = true,
  [KLTK_STRING] = true,
  [KLTK_BOOLVAL] = true,
  [KLTK_NIL] = true,
  [KLTK_ID] = true,
  [KLTK_LBRACKET] = true,
  [KLTK_LBRACE] = true,
  [KLTK_YIELD] = true,
  [KLTK_ASYNC] = true,
  [KLTK_CASE] = true,
  [KLTK_INHERIT] = true,
  [KLTK_LPAREN] = true,

  [KLTK_LOCAL] = true,
  [KLTK_LET] = true,
  [KLTK_METHOD] = true,
  [KLTK_IF] = true,
  [KLTK_REPEAT] = true,
  [KLTK_WHILE] = true,
  [KLTK_MATCH] = true,
  [KLTK_FOR] = true,
  [KLTK_RETURN] = true,
  [KLTK_BREAK] = true,
  [KLTK_CONTINUE] = true,
};
