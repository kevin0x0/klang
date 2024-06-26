#ifndef _KLANG_INCLUDE_PARSE_KLPARSER_UTILS_H_
#define _KLANG_INCLUDE_PARSE_KLPARSER_UTILS_H_

#include "include/array/karray.h"
#include "include/parse/klparser.h"
#include "include/parse/klparser_error.h"
#include "include/parse/klparser_recovery.h"
#include <stdbool.h>

#define klparser_karr_pushast(arr, ast)  {              \
  if (kl_unlikely(!karray_push_back(arr, ast))) {       \
    klparser_error_oom(parser, lex);                    \
    if (ast) klast_delete(ast);                         \
  }                                                     \
}

#define klparser_oomifnull(expr) {                      \
  if (kl_unlikely(!(expr)))                             \
    return klparser_error_oom(parser, lex);             \
}

#define klparser_returnifnull(expr) {                   \
  if (kl_unlikely(!(expr)))                             \
    return NULL;                                        \
}


static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind);
/* check whether current token match 'kind', if not, report an error and try to discover.
 * if discovery failed, return false. */
static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind);
KlStrDesc klparser_newtmpid(KlParser* parser, KlLex* lex);
/* tool for clean ast karray when error occurred. */
void klparser_destroy_astarray(KArray* arr);

extern const bool klparser_isexprbegin[KLTK_NTOKEN];
static inline bool klparser_exprbegin(KlLex* lex);


static inline bool klparser_exprbegin(KlLex* lex) {
  return klparser_isexprbegin[kllex_tokkind(lex)];
}

static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expected '%s'", kltoken_desc(kind));
    return klparser_discardto(lex, kind);
  }
  kllex_next(lex);
  return true;
}

static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expected '%s'", kltoken_desc(kind));
    return klparser_discarduntil(lex, kind);
  }
  return true;
}

#endif
