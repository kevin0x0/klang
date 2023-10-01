#include "pargen/include/parser/parser.h"
#include "pargen/include/parser/error.h"
#include "utils/include/array/addr_array.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>


static void kev_pargenparser_statement_declare(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_rules(KevPParserState* parser_state, KevPLexer* lex);
static KevSymbol** kev_pargenparser_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                                       size_t* p_bodylen, KevActionFunc** actfunc);
static KevSymbol* kev_pargenparser_symbol(KevPParserState* parser_state, KevPLexer* lex);
static KevSymbol* kev_pargenparser_symbol_kind(KevPParserState* parser_state, KevPLexer* lex, KevSymbolType kind);
static KevActionFunc* kev_pargenlexer_actfunc(KevPParserState* parser_state, KevPLexer* lex);
static KevPrioPos kev_pargenparser_sympos(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_prio(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_id(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_start(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_end_symbols(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_confhandler(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_algorithm(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_statement_env_assign(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind);
static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind);
static void kev_pargenparser_recover(KevPLexer* lex, int kind);

static char* kev_pargenparser_check_and_get_expr(KevPParserState* parser_state, KevPLexer* lex);

static char* kev_pargenparser_expr(KevPParserState* parser_state, KevPLexer* lex);
static char* kev_pargenparser_expr_concat(KevPParserState* parser_state, KevPLexer* lex);
static char* kev_pargenparser_expr_unit(KevPParserState* parser_state, KevPLexer* lex);

/* wraper */
static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2);
/* this will free body and actfunc when failed */
static KevRule* kev_pargenparser_rule_create_move(KevPParserState* parser_state, KevSymbol* head,
                                                 KevSymbol** body, size_t bodylen, KevActionFunc* actfunc);
/* this will free name when failed */
static KevSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KevSymbolType kind);
static inline void kev_pargenparser_next_nonblank(KevPLexer* lex);

bool kev_pargenparser_init(KevPParserState* parser_state) {
  if (!parser_state) return false;
  parser_state->rules = kev_addrarray_create();
  parser_state->redact = kev_addrarray_create();
  parser_state->end_symbols = kev_addrarray_create();
  parser_state->symbols = kev_strxmap_create(64);
  parser_state->env_var = kev_strmap_create(16);
  parser_state->priorities = kev_priomap_create(32);
  parser_state->default_symbol_nt = kev_lr_symbol_create(KEV_LR_NONTERMINAL, NULL);
  parser_state->default_symbol_t = kev_lr_symbol_create(KEV_LR_TERMINAL, NULL);
  parser_state->confhandlers = kev_addrarray_create();
  parser_state->start = NULL;
  parser_state->next_symbol_id = 0;
  parser_state->err_count = 0;
  parser_state->next_priority = 0;
  parser_state->algorithm = kev_str_copy("lalr");
  if (!parser_state->rules || !parser_state->symbols || !parser_state->redact ||
      !parser_state->env_var || !parser_state->default_symbol_nt || !parser_state->default_symbol_t ||
      !parser_state->priorities || !parser_state->end_symbols || !parser_state->algorithm ||
      !parser_state->confhandlers) {
    kev_addrarray_delete(parser_state->rules);
    kev_addrarray_delete(parser_state->redact);
    kev_addrarray_delete(parser_state->end_symbols);
    kev_strxmap_delete(parser_state->symbols);
    kev_strmap_delete(parser_state->env_var);
    kev_priomap_delete(parser_state->priorities);
    kev_lr_symbol_delete(parser_state->default_symbol_nt);
    kev_lr_symbol_delete(parser_state->default_symbol_t);
    kev_addrarray_delete(parser_state->confhandlers);
    free(parser_state->algorithm);
    parser_state->rules = NULL;
    parser_state->symbols = NULL;
    parser_state->end_symbols = NULL;
    parser_state->redact = NULL;
    parser_state->env_var = NULL;
    parser_state->priorities = NULL;
    parser_state->default_symbol_t = NULL;
    parser_state->default_symbol_nt = NULL;
    parser_state->confhandlers = NULL;
    parser_state->algorithm = NULL;
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

  for (size_t i = 0; i < kev_addrarray_size(parser_state->confhandlers); ++i) {
    KevConfHandler* handler = (KevConfHandler*)kev_addrarray_visit(parser_state->confhandlers, i);
    free (handler->handler_name);
    free (handler->attribute);
    free(handler);
  }
  kev_addrarray_delete(parser_state->rules);
  kev_addrarray_delete(parser_state->redact);
  kev_addrarray_delete(parser_state->end_symbols);
  kev_strxmap_delete(parser_state->symbols);
  kev_strmap_delete(parser_state->env_var);
  kev_priomap_delete(parser_state->priorities);
  kev_lr_symbol_delete(parser_state->default_symbol_nt);
  kev_lr_symbol_delete(parser_state->default_symbol_t);
  kev_addrarray_delete(parser_state->confhandlers);
  free(parser_state->algorithm);
  parser_state->rules = NULL;
  parser_state->symbols = NULL;
  parser_state->end_symbols = NULL;
  parser_state->redact = NULL;
  parser_state->env_var = NULL;
  parser_state->priorities = NULL;
  parser_state->default_symbol_t = NULL;
  parser_state->default_symbol_nt = NULL;
  parser_state->confhandlers = NULL;
  parser_state->algorithm = NULL;
}

bool kev_pargenparser_parse_file(KevPParserState* parser_state, const char* filepath) {
  FILE* file = fopen(filepath, "r");
  if (!file) return false;
  KevPLexer lex;
  if (!kev_pargenlexer_init(&lex, file)) {
    fclose(file);
    return false;
  }
  kev_pargenlexer_next(&lex);
  kev_pargenparser_parse(parser_state, &lex);
  kev_pargenlexer_destroy(&lex);
  fclose(file);
  return true;
}

void kev_pargenparser_parse(KevPParserState* parser_state, KevPLexer* lex) {
  parser_state->err_count = 0;
  while (lex->currtoken.kind == KEV_PTK_BLKS)
    kev_pargenparser_next_nonblank(lex);
  while (lex->currtoken.kind != KEV_PTK_END) {
    if (lex->currtoken.kind == KEV_PTK_ID) {
      kev_pargenparser_rules(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_DECL) {
      kev_pargenparser_statement_declare(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_SET) {
      kev_pargenparser_statement_set(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_ENV) {
        kev_pargenparser_statement_env_assign(parser_state, lex);
    } else {
      kev_error_report(lex, "expected: ", "identifier or keyword decl");
      parser_state->err_count++;
    }
  }
  parser_state->err_count += lex->err_count;
}

static void kev_pargenparser_rules(KevPParserState* parser_state, KevPLexer* lex) {
  KevSymbol* head = kev_pargenparser_symbol_kind(parser_state, lex, KEV_LR_NONTERMINAL);
  if (kev_lr_symbol_get_type(head) != KEV_LR_NONTERMINAL) {
    kev_error_report(lex, "expected: ", "terminal symbol");
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    return;
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_COLON);
  if (lex->currtoken.kind != KEV_PTK_BAR && lex->currtoken.kind != KEV_PTK_SEMI &&
      lex->currtoken.kind != KEV_PTK_ID) {
    kev_error_report(lex, "unexpected: ", kev_pargenlexer_info(lex->currtoken.kind));
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    return;
  }
  while (true) {
    size_t bodylen = 0;
    KevActionFunc* actfunc = NULL;
    KevSymbol** rulebody = kev_pargenparser_rulebody(parser_state, lex, &bodylen, &actfunc);
    if (!kev_pargenparser_rule_create_move(parser_state, head, rulebody, bodylen, actfunc)) {
      kev_error_report(lex, "out of memory", "");
      parser_state->err_count++;
    }
    if (lex->currtoken.kind != KEV_PTK_BAR)
      break;
    else
      kev_pargenparser_next_nonblank(lex);
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KevSymbol** kev_pargenparser_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                             size_t* p_bodylen, KevActionFunc** p_actfunc) {
  KevAddrArray symarr;
  *p_actfunc = NULL;
  if (!kev_addrarray_init(&symarr)) {
    kev_error_report(lex, "out of memory", NULL);
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    kev_pargenparser_next_nonblank(lex);
    *p_bodylen = 0;
    return NULL;
  }
  while (true) {
    if (lex->currtoken.kind == KEV_PTK_ID) {
      KevSymbol* sym = kev_pargenparser_symbol(parser_state, lex);
      if (!kev_addrarray_push_back(&symarr, sym)) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
    } else if (lex->currtoken.kind == KEV_PTK_STR || lex->currtoken.kind == KEV_PTK_LBE) {
      KevActionFunc* actfunc = kev_pargenlexer_actfunc(parser_state, lex);
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

static KevSymbol* kev_pargenparser_symbol(KevPParserState* parser_state, KevPLexer* lex) {
  char* symbol_name = lex->currtoken.attr.str;
  KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
  KevSymbol* symbol = NULL;
  if (!node) {
    kev_error_report(lex, "undefined symbol: ", symbol_name);
    parser_state->err_count++;
    symbol = parser_state->default_symbol_nt;
  } else {
    symbol = (KevSymbol*)node->value;
  }
  kev_pargenlexer_free_attr(lex);
  kev_pargenparser_next_nonblank(lex);
  return symbol;
}

static KevSymbol* kev_pargenparser_symbol_kind(KevPParserState* parser_state, KevPLexer* lex, KevSymbolType kind) {
  char* symbol_name = lex->currtoken.attr.str;
  KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
  KevSymbol* symbol = NULL;
  if (!node) {
    kev_error_report(lex, "undefined symbol: ", symbol_name);
    parser_state->err_count++;
    symbol = kind == KEV_LR_TERMINAL ? parser_state->default_symbol_t : parser_state->default_symbol_nt;
  } else {
    symbol = (KevSymbol*)node->value;
  }
  if (kev_lr_symbol_get_type(symbol) != kind) {
    kev_error_report(lex, "expected: ", kind == KEV_LR_TERMINAL ? "terminal" : "non-terminal");
    parser_state->err_count++;
    symbol = kind == KEV_LR_TERMINAL ? parser_state->default_symbol_t : parser_state->default_symbol_nt;
  }
  kev_pargenlexer_free_attr(lex);
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
    kev_pargenparser_recover(lex, kind);
  }
}

static void kev_pargenparser_recover(KevPLexer* lex, int kind) {
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
  kev_lr_rule_set_id(rule, kev_addrarray_size(parser_state->rules) - 1);
  return rule;
}

static KevSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KevSymbolType kind) {
  KevSymbol* symbol = kev_lr_symbol_create_move(kind, name);
  if (!symbol) {
    free(name);
    return NULL;
  }

  if (!kev_strxmap_insert(parser_state->symbols, name, symbol)) {
    kev_lr_symbol_delete(symbol);
    return NULL;
  }
  kev_lr_symbol_set_id(symbol, parser_state->next_symbol_id++);
  return symbol;
}

static inline void kev_pargenparser_next_nonblank(KevPLexer* lex) {
  do {
    kev_pargenlexer_next(lex);
  } while (lex->currtoken.kind == KEV_PTK_BLKS);
}

KevActionFunc* kev_pargenlexer_actfunc(KevPParserState* parser_state, KevPLexer* lex) {
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
    } else {
      kev_pargenlexer_free_attr(lex);
    }
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RBE);
  } else {
    char* content = kev_pargenparser_expr(parser_state, lex);
    if (actfunc) {
      actfunc->content = content;
      actfunc->type = KEV_ACTFUNC_FUNCDEF;
    } else {
      free(content);
    }
  }

  return actfunc;
}

static void kev_pargenparser_statement_declare(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  KevSymbolType symbol_kind = KEV_LR_NONTERMINAL;
  if (kev_str_is_prefix(lex->currtoken.attr.str, "terminal")) {
    symbol_kind = KEV_LR_TERMINAL;
  } else if (!kev_str_is_prefix(lex->currtoken.attr.str, "nonterminal") &&
             !kev_str_is_prefix(lex->currtoken.attr.str, "non-terminal")) {
    kev_error_report(lex, "unexpected: ", lex->currtoken.attr.str);
    parser_state->err_count++;
  }

  kev_pargenlexer_free_attr(lex);
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

static void kev_pargenparser_statement_set(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  if (kev_str_is_prefix(lex->currtoken.attr.str, "priority")) {
    kev_pargenparser_statement_set_prio(parser_state, lex);
  } else if (kev_str_is_prefix(lex->currtoken.attr.str, "id")) {
    kev_pargenparser_statement_set_id(parser_state, lex);
  } else if (kev_str_is_prefix(lex->currtoken.attr.str, "start")) {
    kev_pargenparser_statement_set_start(parser_state, lex);
  } else if (kev_str_is_prefix(lex->currtoken.attr.str, "end")) {
    kev_pargenparser_statement_set_end_symbols(parser_state, lex);
  } else if (kev_str_is_prefix(lex->currtoken.attr.str, "conflict-handler")) {
    kev_pargenparser_statement_set_confhandler(parser_state, lex);
  } else if (kev_str_is_prefix(lex->currtoken.attr.str, "algorithm")) {
    kev_pargenparser_statement_set_algorithm(parser_state, lex);
  } else {
    kev_error_report(lex, "unknown attribute: ", lex->currtoken.attr.str);
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    kev_pargenparser_next_nonblank(lex);
  }
}

static void kev_pargenparser_statement_set_prio(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenlexer_free_attr(lex);
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    /* parser_state->default_symbol is returned when the read symbol is not defined */
    KevSymbol* symbol = kev_pargenparser_symbol(parser_state, lex);
    KevPrioPos sympos = kev_pargenparser_sympos(parser_state, lex);
    bool keep_priority = false;
    if (lex->currtoken.kind == KEV_PTK_EQUAL)
      keep_priority = true;
    else if (lex->currtoken.kind != KEV_PTK_LESS  &&
             lex->currtoken.kind != KEV_PTK_COMMA &&
             lex->currtoken.kind != KEV_PTK_SEMI) {
      kev_error_report(lex, "expected: ", "= or <");
      parser_state->err_count++;
      kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    }

    /* only set priority when the symbol is defined */
    if (symbol != parser_state->default_symbol_nt) {
      if (!kev_priomap_search(parser_state->priorities, symbol, sympos)) {
        /* previously the priority of the symbol in this position is not defined */
        if (!kev_priomap_insert(parser_state->priorities, symbol, sympos, parser_state->next_priority)) {
          kev_error_report(lex, "out of memory", NULL);
            parser_state->err_count++;
        }
        if (!keep_priority)
          parser_state->next_priority++;
      } else {  /* it has been defined */
        kev_error_report(lex, "redefined priority: ", kev_lr_symbol_get_name(symbol));
        parser_state->err_count++;
      }
    }
    if (lex->currtoken.kind != KEV_PTK_SEMI)
      kev_pargenparser_next_nonblank(lex);
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_id(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    KevSymbol* symbol = kev_pargenparser_symbol(parser_state, lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_ASSIGN);
    kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_NUM);
    symbol->id = lex->currtoken.attr.num;
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind != KEV_PTK_SEMI)
      kev_pargenparser_match(parser_state, lex, KEV_PTK_COMMA);
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_start(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  KevSymbol* start = kev_pargenparser_symbol_kind(parser_state, lex, KEV_LR_NONTERMINAL);
  if (parser_state->start && parser_state->start != parser_state->default_symbol_nt) {
    kev_error_report(lex, "redefine start symbol, previously defined as: ", kev_lr_symbol_get_name(parser_state->start));
    parser_state->err_count++;
  }
  parser_state->start = start;
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_end_symbols(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    KevSymbol* symbol = kev_pargenparser_symbol_kind(parser_state, lex, KEV_LR_TERMINAL);
    if (!kev_addrarray_push_back(parser_state->end_symbols, symbol)) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
    }
    if (lex->currtoken.kind != KEV_PTK_SEMI)
      kev_pargenparser_match(parser_state, lex, KEV_PTK_COMMA);
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_confhandler(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    char* handler_name = lex->currtoken.attr.str;
    char* attribute = NULL;
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind == KEV_PTK_COLON) {
      kev_pargenparser_next_nonblank(lex);
      attribute = kev_pargenparser_check_and_get_expr(parser_state, lex);
    }
    KevConfHandler* handler = (KevConfHandler*)malloc(sizeof (KevConfHandler));
    if (!handler) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
      free(handler_name);
      free(attribute);
    } else {
      handler->handler_name = handler_name;
      handler->attribute = attribute;
      if (!kev_addrarray_push_back(parser_state->confhandlers, handler)) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
        free(handler_name);
        free(attribute);
        free(handler);
      }
    }
    if (lex->currtoken.kind != KEV_PTK_SEMI)
      kev_pargenparser_match(parser_state, lex, KEV_PTK_COMMA);
  }
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_algorithm(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  free(parser_state->algorithm);
  parser_state->algorithm = lex->currtoken.attr.str;
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KevPrioPos kev_pargenparser_sympos(KevPParserState* parser_state, KevPLexer* lex) {
  KevPrioPos sympos = 1;
  if (lex->currtoken.kind == KEV_PTK_LP) {
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind == KEV_PTK_ID) {
      if (kev_str_is_prefix(lex->currtoken.attr.str, "prefix")) {
        sympos = 0;
      } else if (kev_str_is_prefix(lex->currtoken.attr.str, "postfix")) {
        sympos = KEV_LR_PRIOPOS_POSTFIX;
      } else {
        kev_error_report(lex, "unknown position", NULL);
        parser_state->err_count++;
      }
    } else if (lex->currtoken.kind == KEV_PTK_NUM) {
      sympos = lex->currtoken.attr.num;
    } else {
      kev_error_report(lex, "expected position(positive integer)", NULL);
      parser_state->err_count++;
    }
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RP);
  }
  return sympos;
}

static void kev_pargenparser_statement_env_assign(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  char* id = lex->currtoken.attr.str;
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_ASSIGN);
  char* value = kev_pargenparser_expr(parser_state, lex);
  if (value) {
    if (!kev_strmap_update_move(parser_state->env_var, id, value)) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
      free(value);
    }
  }
  free(id);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static char* kev_pargenparser_expr(KevPParserState* parser_state, KevPLexer* lex) {
  return kev_pargenparser_expr_concat(parser_state, lex);
}

static char* kev_pargenparser_expr_concat(KevPParserState* parser_state, KevPLexer* lex) {
  char* ret = kev_pargenparser_expr_unit(parser_state, lex);
  while (lex->currtoken.kind == KEV_PTK_CONCAT) {
    kev_pargenparser_next_nonblank(lex);
    char* rhs = kev_pargenparser_expr_unit(parser_state, lex);
    if (!rhs || !ret) {
      free(rhs);
      free(ret);
      ret = NULL;
    } else {
      char* result = kev_str_concat(ret, rhs);
      free(ret);
      if (!result) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
        free(rhs);
      }
      ret = result;
    }
  }
  return ret;
}

static char* kev_pargenparser_expr_unit(KevPParserState* parser_state, KevPLexer* lex) {
  if (lex->currtoken.kind == KEV_PTK_ID) {
    KevStringMapNode* node = kev_strmap_search(parser_state->env_var, lex->currtoken.attr.str);
    if (!node) {
      kev_error_report(lex, "undefined variable: ", lex->currtoken.attr.str);
      parser_state->err_count++;
      kev_pargenparser_next_nonblank(lex);
      return NULL;
    } else {
      char* ret = kev_str_copy(node->value);
      if (!ret) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
      kev_pargenparser_next_nonblank(lex);
      return ret;
    }
  } else {
    char* ret = lex->currtoken.attr.str;
    kev_pargenparser_next_nonblank(lex);
    return ret;
  }
}

static char* kev_pargenparser_check_and_get_expr(KevPParserState* parser_state, KevPLexer* lex) {
  if (!(lex->currtoken.kind == KEV_PTK_STR || lex->currtoken.kind == KEV_PTK_ID)) {
    kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  }
  return kev_pargenparser_expr(parser_state, lex);
}
