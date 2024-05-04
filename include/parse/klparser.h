#ifndef KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H
#define KEVCC_KLANG_INCLUDE_PARSE_KLPARSER_H

#include "include/ast/klast.h"
#include "include/ast/klast.h"
#include "include/parse/kllex.h"

typedef struct tagKlParser {
  char* inputname;
  KlStrTbl* strtbl;
  size_t incid;
  KlError* klerror;
  struct {
    KlStrDesc this;
  } string;
} KlParser;


bool klparser_init(KlParser* parser, KlStrTbl* strtbl, char* inputname, KlError* klerror);

static inline bool klparser_match(KlParser* parser, KlLex* lex, KlTokenKind kind);
/* check whether current token match 'kind', if not, report an error and try to discover.
 * if discovery failed, return false. */
static inline bool klparser_check(KlParser* parser, KlLex* lex, KlTokenKind kind);

bool klparser_discarduntil(KlLex* lex, KlTokenKind kind);
bool klparser_discardto(KlLex* lex, KlTokenKind kind);


static inline KlAst* klparser_expr(KlParser* parser, KlLex* lex);
KlAst* klparser_exprwhere(KlParser* parser, KlLex* lex, KlAst* expr);
KlAst* klparser_exprunit(KlParser* parser, KlLex* lex, bool* inparenthesis);
KlAst* klparser_exprpost(KlParser* parser, KlLex* lex);
KlAst* klparser_exprpre(KlParser* parser, KlLex* lex);
KlAst* klparser_exprbin(KlParser* parser, KlLex* lex, int prio);

KlAst* klparser_stmt(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtlet(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtassign(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtexpr(KlParser* parser, KlLex* lex);
KlAstStmtIf* klparser_stmtif(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtfor(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtwhile(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtblock(KlParser* parser, KlLex* lex);
/* do not set file position */
KlAstStmtList* klparser_stmtlist(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtrepeat(KlParser* parser, KlLex* lex);
KlAst* klparser_stmtreturn(KlParser* parser, KlLex* lex);
static inline KlAst* klparser_stmtbreak(KlParser* parser, KlLex* lex);
static inline KlAst* klparser_stmtcontinue(KlParser* parser, KlLex* lex);




void klparser_error(KlParser* parser, Ki* input, KlFileOffset begin, KlFileOffset end, const char* format, ...);
static void* klparser_error_oom(KlParser* parser, KlLex* lex);

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

static inline KlAst* klparser_expr(KlParser* parser, KlLex* lex) {
  KlAst* expr = klparser_exprbin(parser, lex, 0);
  if (kl_unlikely(!expr)) return NULL;
  return kllex_check(lex, KLTK_WHERE) ? klparser_exprwhere(parser, lex, expr) :
                                        expr;
}

static inline KlAst* klparser_stmtbreak(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_BREAK), "expect 'break'");
  KlAstStmtBreak* stmtbreak = klast_stmtbreak_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtbreak)) return klparser_error_oom(parser, lex);
  return klcast(KlAst*, stmtbreak);
}

static inline KlAst* klparser_stmtcontinue(KlParser* parser, KlLex* lex) {
  kl_assert(kllex_check(lex, KLTK_CONTINUE), "expect 'continue'");
  KlAstStmtContinue* stmtcontinue = klast_stmtcontinue_create(lex->tok.begin, lex->tok.end);
  kllex_next(lex);
  if (kl_unlikely(!stmtcontinue)) return klparser_error_oom(parser, lex);
  return klcast(KlAst*, stmtcontinue);
}

static void* klparser_error_oom(KlParser* parser, KlLex* lex) {
  klparser_error(parser, kllex_inputstream(lex), lex->tok.begin, lex->tok.end, "out of memory");
  return NULL;
}

#endif
