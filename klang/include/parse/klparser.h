#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H

#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_stmt.h"
#include "klang/include/parse/kllex.h"

typedef struct tagKlParser {
  Ko* err;
  char* inputname;
  size_t errcount;
  KlStrTab* strtab;
  struct {
    unsigned int tabstop;
    char curl;
  } config;
} KlParser;



static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind);
/* check whether current token match 'kind', if not, report an error and try to discover.
 * if discovery failed, return false. */
static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind);

bool klparser_discarduntil(KlLex* lex, KlTokenKind kind);
bool klparser_discardto(KlLex* lex, KlTokenKind kind);


static inline KlCst* klparser_expr(KlParser* parser, KlLex* lex);
KlCst* klparser_exprunit(KlParser* parser, KlLex* lex);
KlCst* klparser_exprpost(KlParser* parser, KlLex* lex);
KlCst* klparser_exprpre(KlParser* parser, KlLex* lex);
KlCst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio);
KlCst* klparser_exprter(KlParser* parser, KlLex* lex);

KlCst* klparser_stmt(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtlet(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtassign(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtexpr(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtif(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtfor(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtwhile(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtblock(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtlist(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtrepeat(KlParser* parser, KlLex* lex);
KlCst* klparser_stmtreturn(KlParser* parser, KlLex* lex);
static inline KlCst* klparser_stmtbreak(KlParser* parser, KlLex* lex);
static inline KlCst* klparser_stmtcontinue(KlParser* parser, KlLex* lex);




void klparser_error(KlParser* parser, Ki* input, KlFilePos begin, KlFilePos end, const char* format, ...);
static KlCst* klparser_error_oom(KlParser* parser, KlLex* lex);

static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expect '%s'", kltoken_desc(kind));
    return klparser_discardto(lex, kind);
  }
  kllex_next(lex);
  return true;
}

static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind) {
  if (kl_unlikely(lex->tok.kind != kind)) {
    klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "expect '%s'", kltoken_desc(kind));
    return klparser_discarduntil(lex, kind);
  }
  return true;
}

static inline KlCst* klparser_expr(KlParser* parser, KlLex* lex) {
  return klparser_exprter(parser, lex);
}

static inline KlCst* klparser_stmtbreak(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_BREAK), "expect 'break'");
  KlFilePos begin = lex->tok.begin;
  KlFilePos end = lex->tok.end;
  kllex_next(lex);
  KlCstStmtBreak* stmtbreak = klcst_stmtbreak_create();
  if (kl_unlikely(!stmtbreak)) return klparser_error_oom(parser, lex);
  klcst_setposition(klcast(KlCst*, stmtbreak), begin, end);
  return klcast(KlCst*, stmtbreak);
}

static inline KlCst* klparser_stmtcontinue(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CONTINUE), "expect 'continue'");
  KlFilePos begin = lex->tok.begin;
  KlFilePos end = lex->tok.end;
  kllex_next(lex);
  KlCstStmtContinue* stmtcontinue = klcst_stmtcontinue_create();
  if (kl_unlikely(!stmtcontinue)) return klparser_error_oom(parser, lex);
  klcst_setposition(klcast(KlCst*, stmtcontinue), begin, end);
  return klcast(KlCst*, stmtcontinue);
}

static KlCst* klparser_error_oom(KlParser* parser, KlLex* lex) {
  klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "out of memory");
  return NULL;
}

#endif
