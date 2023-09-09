#include "pargen/include/parser/parser.h"
#include "pargen/include/parser/error.h"
#include "pargen/include/parser/lexer.h"
#include "kevlr/include/lr.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>


static void kev_pargenparser_statement_rule(KevPParserState* parser_state, KevPLexer* lex);
static KevSymbol** kev_pargenparser_statement_rulebody(KevPParserState* parser_state, KevPLexer* lex, size_t* p_bodylen);
static KevSymbol* kev_pargenparser_statement_symbol(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind);
static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind);
static void kev_pargenparser_recover(KevPParserState* parser_state, KevPLexer* lex, int kind);

/* wraper */
static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2);

bool kev_pargenparser_init(KevPParserState* parser_state) {
  if (!parser_state) return false;
  parser_state->rules = kev_addrarray_create();
  parser_state->redact = kev_addrarray_create();
  parser_state->symbols = kev_strxmap_create(64);
  parser_state->env_var = kev_strmap_create(16);
  parser_state->default_symbol = kev_lr_symbol_create(KEV_LR_NONTERMINAL, NULL);
  parser_state->err_count = 0;
  parser_state->next_priority = 0;
  if (!parser_state->rules || !parser_state->symbols || !parser_state->redact ||
      !parser_state->env_var || !parser_state->default_symbol) {
    kev_addrarray_delete(parser_state->rules);
    kev_addrarray_delete(parser_state->redact);
    kev_strxmap_delete(parser_state->symbols);
    kev_strmap_delete(parser_state->env_var);
    kev_lr_symbol_delete(parser_state->default_symbol);
    parser_state->rules = NULL;
    parser_state->symbols = NULL;
    parser_state->redact = NULL;
    parser_state->env_var = NULL;
    return false;
  }
  return true;
}

void kev_pargenparser_destroy(KevPParserState* parser_state) {
  if (!parser_state) return;
  kev_addrarray_delete(parser_state->rules);
  kev_addrarray_delete(parser_state->redact);
  kev_strxmap_delete(parser_state->symbols);
  kev_strmap_delete(parser_state->env_var);
  kev_lr_symbol_delete(parser_state->default_symbol);
  parser_state->rules = NULL;
  parser_state->symbols = NULL;
  parser_state->redact = NULL;
  parser_state->env_var = NULL;
  parser_state->default_symbol = NULL;
}

void kev_pargenparser_parse(KevPParserState* parser_state, KevPLexer* lex) {

}

static void kev_pargenparser_statement_rule(KevPParserState* parser_state, KevPLexer* lex) {
  KevSymbol* head = kev_pargenparser_statement_symbol(parser_state, lex);
  if (kev_lr_symbol_get_type(head) != KEV_LR_NONTERMINAL) {
    kev_error_report(lex, "expected: ", "terminal symbol");
    parser_state->err_count++;
    kev_pargenparser_recover(parser_state, lex, KEV_PTK_SEMI);
    return;
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_COLON);
  if (lex->currtoken.kind != KEV_PTK_BAR && lex->currtoken.kind != KEV_PTK_SEMI &&
      lex->currtoken.kind != KEV_PTK_ID) {
    kev_error_report(lex, "unexpected: ", kev_pargenlexer_info(lex->currtoken.kind));
    parser_state->err_count++;
    kev_pargenparser_recover(parser_state, lex, KEV_PTK_SEMI);
    return;
  }
  do {
    size_t bodylen = 0;
    KevSymbol** rulebody = kev_pargenparser_statement_rulebody(parser_state, lex, &bodylen);
    KevRule* rule = kev_lr_rule_create_move(head, rulebody, bodylen);
    if (!rule) {
      kev_error_report(lex, "out of memory", "");
      parser_state->err_count++;
    } else {
      kev_addrarray_push_back(parser_state->rules, rule);
    }
  } while (lex->currtoken.kind == KEV_PTK_BAR);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KevSymbol** kev_pargenparser_statement_rulebody(KevPParserState* parser_state, KevPLexer* lex, size_t* p_bodylen) {
  KevAddrArray symarr;
  if (!kev_addrarray_init(&symarr)) {
    kev_error_report(lex, "out of memory", NULL);
    parser_state->err_count++;
    kev_pargenparser_recover(parser_state, lex, KEV_PTK_SEMI);
    kev_pargenlexer_next(lex);
    *p_bodylen = 0;
    return NULL;
  }
  while (lex->currtoken.kind == KEV_PTK_ID) {
    KevSymbol* sym = kev_pargenparser_statement_symbol(parser_state, lex);
    if (!kev_addrarray_push_back(&symarr, sym)) {
      kev_addrarray_delete(&symarr);
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
      kev_pargenparser_recover(parser_state, lex, KEV_PTK_SEMI);
      kev_pargenlexer_next(lex);
      *p_bodylen = 0;
      return NULL;
    }
  }
  *p_bodylen = kev_addrarray_size(&symarr);
  KevSymbol** body = (KevSymbol**)symarr.begin;
  symarr.begin = NULL;
  symarr.current = NULL;
  symarr.end = NULL;
  return body;
}

static KevSymbol* kev_pargenparser_statement_symbol(KevPParserState* parser_state, KevPLexer* lex) {
  char* symbol_name = lex->currtoken.attr.str;
  KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
  KevSymbol* symbol = NULL;
  if (!node) {
    kev_error_report(lex, "undefined symbol: ", symbol_name);
    parser_state->err_count++;
    symbol = parser_state->default_symbol;
  } else {
    symbol = (KevSymbol*)node->value;
  }
  kev_pargenlexer_next(lex);
  return symbol;
}

static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2) {
  kev_parser_throw_error(stderr, lex->infile, lex->currtoken.begin, info1, info2);
}

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  kev_pargenparser_guarantee(parser_state, lex, kind);
  kev_pargenlexer_next(lex);
}

static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  if (lex->currtoken.kind != kind) {
    kev_error_report(lex, "expected: ", kev_pargenlexer_info(kind));
    kev_pargenparser_recover(parser_state, lex, kind);
  }
}

static void kev_pargenparser_recover(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  while (lex->currtoken.kind != kind && lex->currtoken.kind != KEV_PTK_END)
    kev_pargenlexer_next(lex);
  if (lex->currtoken.kind != kind) {
    kev_error_report(lex, "can not recover form previous error", "");
    exit(EXIT_FAILURE);
  }
}
