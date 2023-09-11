#include "pargen/include/parser/parser.h"
#include "pargen/include/parser/error.h"
#include "kevlr/include/lr.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>


static void kev_pargenparser_statement_declare(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_rules(KevPParserState* parser_state, KevPLexer* lex);
static KevSymbol** kev_pargenparser_statement_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                                       size_t* p_bodylen, KevActionFunc** actfunc);
static KevSymbol* kev_pargenparser_statement_symbol(KevPParserState* parser_state, KevPLexer* lex);
static KevActionFunc* kev_pargenlexer_statement_actfunc(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind);
static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind);
static void kev_pargenparser_recover(KevPParserState* parser_state, KevPLexer* lex, int kind);

/* wraper */
static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2);
/* this will free body and actfunc when failed */
static KevRule* kev_pargenparser_rule_create_move(KevPParserState* parser_state, KevSymbol* head,
                                                 KevSymbol** body, size_t bodylen, KevActionFunc* actfunc);
/* this will free name when failed */
static KevSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KevSymbolKind kind);
static inline void kev_pargenparser_next_nonblank(KevPLexer* lex);

bool kev_pargenparser_init(KevPParserState* parser_state) {
  if (!parser_state) return false;
  parser_state->rules = kev_addrarray_create();
  parser_state->redact = kev_addrarray_create();
  parser_state->symbols = kev_strxmap_create(64);
  parser_state->env_var = kev_strmap_create(16);
  parser_state->priorities = kev_priomap_create(32);
  parser_state->default_symbol = kev_lr_symbol_create(KEV_LR_NONTERMINAL, NULL);
  parser_state->err_count = 0;
  parser_state->next_priority = 0;
  if (!parser_state->rules || !parser_state->symbols || !parser_state->redact ||
      !parser_state->env_var || !parser_state->default_symbol || !parser_state->priorities) {
    kev_addrarray_delete(parser_state->rules);
    kev_addrarray_delete(parser_state->redact);
    kev_strxmap_delete(parser_state->symbols);
    kev_strmap_delete(parser_state->env_var);
    kev_priomap_delete(parser_state->priorities);
    kev_lr_symbol_delete(parser_state->default_symbol);
    parser_state->rules = NULL;
    parser_state->symbols = NULL;
    parser_state->redact = NULL;
    parser_state->env_var = NULL;
    parser_state->priorities = NULL;
    return false;
  }
  return true;
}

void kev_pargenparser_destroy(KevPParserState* parser_state) {
  if (!parser_state) return;
  /* parser_state->rules and parser_state->redact have same rule number */
  size_t rule_no = kev_addrarray_size(parser_state->rules);
  for (size_t i = 0; i < rule_no; ++i) {
    kev_lr_rule_delete((KevRule*)kev_addrarray_visit(parser_state->rules, i));
    KevActionFunc* actfunc = (KevActionFunc*)kev_addrarray_visit(parser_state->redact, i);
    if (actfunc) {
      free(actfunc->content);
      free(actfunc);
    }
  }
  for (KevStrXMapNode* node = kev_strxmap_iterate_begin(parser_state->symbols);
       node != NULL;
       node = kev_strxmap_iterate_next(parser_state->symbols, node)) {
    kev_lr_symbol_delete((KevSymbol*)node->value);
  }
  kev_addrarray_delete(parser_state->rules);
  kev_addrarray_delete(parser_state->redact);
  kev_strxmap_delete(parser_state->symbols);
  kev_strmap_delete(parser_state->env_var);
  kev_priomap_delete(parser_state->priorities);
  kev_lr_symbol_delete(parser_state->default_symbol);
  parser_state->rules = NULL;
  parser_state->symbols = NULL;
  parser_state->redact = NULL;
  parser_state->env_var = NULL;
  parser_state->priorities = NULL;
  parser_state->default_symbol = NULL;
  parser_state->err_count = 0;
  parser_state->next_priority = 0;
}

void kev_pargenparser_parse(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  while (lex->currtoken.kind != KEV_PTK_END) {
    if (lex->currtoken.kind == KEV_PTK_ID) {
      kev_pargenparser_statement_rules(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_DECL) {
      kev_pargenparser_statement_declare(parser_state, lex);
    } else {
      kev_error_report(lex, "expected: ", "identifier or keyword decl");
    }
  }
}

static void kev_pargenparser_statement_rules(KevPParserState* parser_state, KevPLexer* lex) {
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
    KevActionFunc* actfunc = NULL;
    KevSymbol** rulebody = kev_pargenparser_statement_rulebody(parser_state, lex, &bodylen, &actfunc);
    if (!kev_pargenparser_rule_create_move(parser_state, head, rulebody, bodylen, actfunc)) {
      kev_error_report(lex, "out of memory", "");
      parser_state->err_count++;
    }
  } while (lex->currtoken.kind == KEV_PTK_BAR);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KevSymbol** kev_pargenparser_statement_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                                       size_t* p_bodylen, KevActionFunc** p_actfunc) {
  KevAddrArray symarr;
  *p_actfunc = NULL;
  if (!kev_addrarray_init(&symarr)) {
    kev_error_report(lex, "out of memory", NULL);
    parser_state->err_count++;
    kev_pargenparser_recover(parser_state, lex, KEV_PTK_SEMI);
    kev_pargenparser_next_nonblank(lex);
    *p_bodylen = 0;
    return NULL;
  }
  while (true) {
    if (lex->currtoken.kind == KEV_PTK_ID) {
      KevSymbol* sym = kev_pargenparser_statement_symbol(parser_state, lex);
      if (!kev_addrarray_push_back(&symarr, sym)) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
    } else if (lex->currtoken.kind == KEV_PTK_STR || lex->currtoken.kind == KEV_PTK_LBE) {
      KevActionFunc* actfunc = kev_pargenlexer_statement_actfunc(parser_state, lex);
      if (lex->currtoken.kind == KEV_PTK_LBE ||
          lex->currtoken.kind == KEV_PTK_STR ||
          lex->currtoken.kind == KEV_PTK_ID) {
        KevSymbol* act_head = kev_pargenparser_symbol_create_move(parser_state, NULL, KEV_LR_NONTERMINAL);
        if (!act_head                                                                    ||
            !kev_pargenparser_rule_create_move(parser_state, act_head, NULL, 0, actfunc) ||
            !kev_addrarray_push_back(&symarr, act_head)) {
          kev_error_report(lex, "out of memory", NULL);
          parser_state->err_count++;
        }
      } else {
        *p_actfunc = actfunc;
      }
    } else {
      break;
    }
  }
  *p_bodylen = kev_addrarray_size(&symarr);
  KevSymbol** body = (KevSymbol**)kev_addrarray_steal(&symarr);
  kev_addrarray_destroy(&symarr);
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
  kev_pargenparser_next_nonblank(lex);
  return symbol;
}

static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2) {
  kev_parser_throw_error(stderr, lex->infile, lex->currtoken.begin, info1, info2);
}

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  kev_pargenparser_guarantee(parser_state, lex, kind);
  kev_pargenparser_next_nonblank(lex);
}

static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  if (lex->currtoken.kind != kind) {
    kev_error_report(lex, "expected: ", kev_pargenlexer_info(kind));
    parser_state->err_count++;
    kev_pargenparser_recover(parser_state, lex, kind);
  }
}

static void kev_pargenparser_recover(KevPParserState* parser_state, KevPLexer* lex, int kind) {
  while (lex->currtoken.kind != kind && lex->currtoken.kind != KEV_PTK_END) {
    kev_pargenlexer_free_attr(lex);
    kev_pargenlexer_next(lex);
  }
  if (lex->currtoken.kind != kind) {
    kev_error_report(lex, "can not recover form previous error", "");
    exit(EXIT_FAILURE);
  }
}

static KevRule* kev_pargenparser_rule_create_move(KevPParserState* parser_state, KevSymbol* head,
                                                  KevSymbol** body, size_t bodylen, KevActionFunc* actfunc) {
  KevRule* rule = kev_lr_rule_create_move(head, body, bodylen);
  if (!rule) {
    free(body);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  if (!kev_addrarray_push_back(parser_state->rules, rule)) {
    kev_lr_rule_delete(rule);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  if (!kev_addrarray_push_back(parser_state->redact, actfunc)) {
    kev_addrarray_pop_back(parser_state->rules);
    kev_lr_rule_delete(rule);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  return rule;
}

static KevSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KevSymbolKind kind) {
  KevSymbol* symbol = kev_lr_symbol_create_move(kind, name);
  if (!symbol) {
    free(name);
    return NULL;
  }

  if (!kev_strxmap_insert(parser_state->symbols, name, symbol)) {
    kev_lr_symbol_delete(symbol);
    return NULL;
  }
  return symbol;
}

static inline void kev_pargenparser_next_nonblank(KevPLexer* lex) {
  do {
    kev_pargenlexer_next(lex);
  } while (lex->currtoken.kind == KEV_PTK_BLKS);
}

KevActionFunc* kev_pargenlexer_statement_actfunc(KevPParserState* parser_state, KevPLexer* lex) {
  KevActionFunc* actfunc = (KevActionFunc*)malloc(sizeof (KevActionFunc));
  if (!actfunc) {
    kev_error_report(lex, "out of memory", NULL);
    parser_state->err_count++;
  }
  if (lex->currtoken.kind == KEV_PTK_LBE) {
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
    if (actfunc) {
      actfunc->content = lex->currtoken.attr.str;
      actfunc->type = KEV_ACTFUNC_FUNCNAME;
    }
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RBE);
  } else {
    if (actfunc) {
      actfunc->content = lex->currtoken.attr.str;
      actfunc->type = KEV_ACTFUNC_FUNCDEF;
    }
    kev_pargenparser_next_nonblank(lex);
  }

  return actfunc;
}

static void kev_pargenparser_statement_declare(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  KevSymbolKind symbol_kind = KEV_LR_NONTERMINAL;
  if (kev_str_prefix(lex->currtoken.attr.str, "terminal")) {
    symbol_kind = KEV_LR_TERMINAL;
  } else if (!kev_str_prefix(lex->currtoken.attr.str, "nonterminal") &&
             !kev_str_prefix(lex->currtoken.attr.str, "non-terminal")) {
    kev_error_report(lex, "unexpected: ", lex->currtoken.attr.str);
    parser_state->err_count++;
  }

  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_COLON);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    if (!kev_pargenparser_symbol_create_move(parser_state, lex->currtoken.attr.str, symbol_kind)) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
    }
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind == KEV_PTK_COMMA)
      kev_pargenparser_next_nonblank(lex);
    else if (lex->currtoken.kind != KEV_PTK_SEMI) {
      kev_error_report(lex, "expected: ", "\',\'");
      parser_state->err_count++;
    }
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}
