#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H

#include "klang/include/cst/klcst.h"
#include "klang/include/parse/kllex.h"

typedef struct tagKlParser {
  KlLex* lex;
  Ko* err;
} KlParser;



static inline bool klparser_match(KlParser* parser, KlTokenKind kind);
/* check whether current token match 'kind', if not, report an error and try to discover.
 * if discovery failed, return false. */
static inline bool klparser_check(KlParser* parser, KlTokenKind kind);
static inline bool klparser_trymatch(KlParser* parser, KlTokenKind kind);
static inline void klparser_next(KlParser* parser);

bool klparser_discarduntil(KlParser* parser, KlTokenKind kind);
bool klparser_discardto(KlParser* parser, KlTokenKind kind);


KlCst* klparser_expr(KlParser* parser);
KlCst* klparser_exprunit(KlParser* parser);
KlCst* klparser_exprpost(KlParser* parser);
KlCst* klparser_exprpre(KlParser* parser);
KlCst* klparser_exprbin(KlParser* parser);
KlCst* klparser_exprtri(KlParser* parser);


void klparser_error(KlParser* parser, const char* format, ...);

static inline bool klparser_match(KlParser* parser, KlTokenKind kind) {
  KlLex* lex = parser->lex;
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, "expect %s", kltoken_desc(kind));
    return klparser_discardto(parser, kind);
  }
  kllex_next(lex);
  return true;
}

static inline bool klparser_check(KlParser* parser, KlTokenKind kind) {
  KlLex* lex = parser->lex;
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, "expect %s", kltoken_desc(kind));
    return klparser_discarduntil(parser, kind);
  }
  return true;
}

static inline bool klparser_trymatch(KlParser* parser, KlTokenKind kind) {
  KlLex* lex = parser->lex;
  if (lex->tok.kind == kind) {
    kllex_next(lex);
    return true;
  }
  return false;
}

static inline void klparser_next(KlParser* parser) {
  kllex_next(parser->lex);
}



#endif
