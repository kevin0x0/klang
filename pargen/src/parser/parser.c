#include "pargen/include/parser/parser.h"
#include "pargen/include/parser/error.h"
#include "pargen/include/parser/symtable.h"
#include "utils/include/array/karray.h"
#include "utils/include/os_spec/dir.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>


static KevSymTableNode* kev_pargenparser_symbol_search_rec(KevPParserState* parser_state, const char* key, size_t begin_symtbl_id);

static void kev_pargenparser_statement_declare(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_rules(KevPParserState* parser_state, KevPLexer* lex);
static KlrSymbol** kev_pargenparser_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                                       size_t* p_bodylen, KevActionFunc** actfunc);
static KlrSymbol* kev_pargenparser_symbol(KevPParserState* parser_state, KevPLexer* lex);
static KlrSymbol* kev_pargenparser_symbol_kind(KevPParserState* parser_state, KevPLexer* lex, KlrSymbolKind kind);
static KevActionFunc* kev_pargenparser_actfunc(KevPParserState* parser_state, KevPLexer* lex);
static KlrPrioPos kev_pargenparser_sympos(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_prio(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_id(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_start(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_end_symbols(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_confhandler(KevPParserState* parser_state, KevPLexer* lex);
static void kev_pargenparser_statement_set_algorithm(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_statement_var_assign(KevPParserState* parser_state, KevPLexer* lex);
static size_t kev_pargenparser_statement_var_assign_scope(KevPParserState* parser_state, KevPLexer* lex, const char* varname);
static void kev_pargenparser_statement_import(KevPParserState* parser_state, KevPLexer* lex);

static void kev_pargenparser_match(KevPParserState* parser_state, KevPLexer* lex, int kind);
static inline void kev_pargenparser_guarantee(KevPParserState* parser_state, KevPLexer* lex, int kind);
static void kev_pargenparser_recover(KevPLexer* lex, int kind);

static char* kev_pargenparser_expr(KevPParserState* parser_state, KevPLexer* lex);
static char* kev_pargenparser_expr_concat(KevPParserState* parser_state, KevPLexer* lex);
static char* kev_pargenparser_expr_prefix(KevPParserState* parser_state, KevPLexer* lex);
static char* kev_pargenparser_expr_unit(KevPParserState* parser_state, KevPLexer* lex);
/* this function will free the 'value', There is no need to free 'value' for caller */
static char* kev_pargenparser_expr_postfix(KevPParserState* parser_state, KevPLexer* lex, char* value);

/* wraper */
static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2);
/* this will free body and actfunc when failed */
static KlrRule* kev_pargenparser_rule_create_move(KevPParserState* parser_state, KlrSymbol* head,
                                                 KlrSymbol** body, size_t bodylen, KevActionFunc* actfunc);
/* this will free name when failed */
static KlrSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KlrSymbolKind kind);
static inline void kev_pargenparser_next_nonblank(KevPLexer* lex);

bool kev_pargenparser_init(KevPParserState* parser_state) {
  if (!parser_state) return false;
  KevSymTable* global_symtbl = kev_symtable_create(0, 0);
  parser_state->rules = karray_create();
  parser_state->redact = karray_create();
  parser_state->end_symbols = karray_create();
  parser_state->symbols = kev_strxmap_create(64);
  parser_state->symtables = karray_create();
  karray_push_back(parser_state->symtables, global_symtbl);  /* this never fails */
  parser_state->priorities = klr_priomap_create(32);
  parser_state->default_symbol_nt = klr_symbol_create(KLR_NONTERMINAL, NULL);
  parser_state->default_symbol_t = klr_symbol_create(KLR_TERMINAL, NULL);
  parser_state->confhandlers = karray_create();
  parser_state->start = NULL;
  parser_state->next_symbol_id = 0;
  parser_state->err_count = 0;
  parser_state->next_priority = 0;
  parser_state->curr_symtbl = kev_symtable_get_self_id(global_symtbl);
  parser_state->algorithm = kev_str_copy("lalr");
  if (!parser_state->rules || !parser_state->symbols || !parser_state->redact ||
      !parser_state->symtables || !parser_state->default_symbol_nt || !parser_state->default_symbol_t ||
      !parser_state->priorities || !parser_state->end_symbols || !parser_state->algorithm ||
      !parser_state->confhandlers || !global_symtbl) {
    kev_symtable_delete(global_symtbl);
    karray_delete(parser_state->rules);
    karray_delete(parser_state->redact);
    karray_delete(parser_state->end_symbols);
    kev_strxmap_delete(parser_state->symbols);
    karray_delete(parser_state->symtables);
    klr_priomap_delete(parser_state->priorities);
    klr_symbol_delete(parser_state->default_symbol_nt);
    klr_symbol_delete(parser_state->default_symbol_t);
    karray_delete(parser_state->confhandlers);
    free(parser_state->algorithm);
    parser_state->rules = NULL;
    parser_state->symbols = NULL;
    parser_state->end_symbols = NULL;
    parser_state->redact = NULL;
    parser_state->symtables = NULL;
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
  size_t rule_no = karray_size(parser_state->rules);
  for (size_t i = 0; i < rule_no; ++i) {
    klr_rule_delete((KlrRule*)karray_access(parser_state->rules, i));
    KevActionFunc* actfunc = (KevActionFunc*)karray_access(parser_state->redact, i);
    if (actfunc) {
      free(actfunc->content);
      free(actfunc);
    }
  }
  for (KevStrXMapNode* node = kev_strxmap_iterate_begin(parser_state->symbols);
       node != NULL;
       node = kev_strxmap_iterate_next(parser_state->symbols, node)) {
    klr_symbol_delete((KlrSymbol*)node->value);
  }

  for (size_t i = 0; i < karray_size(parser_state->confhandlers); ++i) {
    KevConfHandler* handler = (KevConfHandler*)karray_access(parser_state->confhandlers, i);
    free (handler->handler_name);
    free (handler->attribute);
    free(handler);
  }

  for (size_t i = 0; i < karray_size(parser_state->symtables); ++i) {
    kev_symtable_delete((KevSymTable*)karray_access(parser_state->symtables, i));
  }
  karray_delete(parser_state->rules);
  karray_delete(parser_state->redact);
  karray_delete(parser_state->end_symbols);
  kev_strxmap_delete(parser_state->symbols);
  karray_delete(parser_state->symtables);
  klr_priomap_delete(parser_state->priorities);
  klr_symbol_delete(parser_state->default_symbol_nt);
  klr_symbol_delete(parser_state->default_symbol_t);
  karray_delete(parser_state->confhandlers);
  free(parser_state->algorithm);
  parser_state->rules = NULL;
  parser_state->symbols = NULL;
  parser_state->end_symbols = NULL;
  parser_state->redact = NULL;
  parser_state->symtables = NULL;
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
  if (!kev_pargenlexer_init(&lex, file, filepath)) {
    fclose(file);
    return false;
  }

  kev_pargenlexer_next(&lex);
  char* file_dir = kev_trunc_leaf(filepath);
  KevSymTable* curr_symtbl = kev_symtable_create(karray_size(parser_state->symtables), parser_state->curr_symtbl);
  if (!curr_symtbl || !karray_push_back(parser_state->symtables, curr_symtbl) ||
      !file_dir || !kev_symtable_insert_move(curr_symtbl, "@file-directory", file_dir)) {
    kev_error_report(&lex, "out of memory", NULL);
    parser_state->err_count++;
    free(file_dir);
    kev_symtable_delete(curr_symtbl);
    if (!karray_top(parser_state->symtables))
      karray_pop_back(parser_state->symtables);
  } else {
    parser_state->curr_symtbl = kev_symtable_get_self_id(curr_symtbl);
    kev_pargenparser_parse(parser_state, &lex);
    parser_state->curr_symtbl = kev_symtable_get_parent_id(curr_symtbl);
  }
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
      kev_pargenparser_statement_rules(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_DECL) {
      kev_pargenparser_statement_declare(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_SET) {
      kev_pargenparser_statement_set(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_LET) {
      kev_pargenparser_statement_var_assign(parser_state, lex);
    } else if (lex->currtoken.kind == KEV_PTK_IMPORT) {
      kev_pargenparser_statement_import(parser_state, lex);
    } else {
      kev_error_report(lex, "unexpected: ", kev_pargenlexer_info(lex->currtoken.kind));
      parser_state->err_count++;
    }
  }
  parser_state->err_count += lex->err_count;
}

static void kev_pargenparser_statement_rules(KevPParserState* parser_state, KevPLexer* lex) {
  KlrSymbol* head = kev_pargenparser_symbol_kind(parser_state, lex, KLR_NONTERMINAL);
  if (klr_symbol_get_kind(head) != KLR_NONTERMINAL) {
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
    KlrSymbol** rulebody = kev_pargenparser_rulebody(parser_state, lex, &bodylen, &actfunc);
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

static KlrSymbol** kev_pargenparser_rulebody(KevPParserState* parser_state, KevPLexer* lex,
                                             size_t* p_bodylen, KevActionFunc** p_actfunc) {
  KArray rulebody;
  *p_actfunc = NULL;
  if (!karray_init(&rulebody)) {
    kev_error_report(lex, "out of memory", NULL);
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    kev_pargenparser_next_nonblank(lex);
    *p_bodylen = 0;
    return NULL;
  }
  while (true) {
    if (lex->currtoken.kind == KEV_PTK_ID) {
      KlrSymbol* sym = kev_pargenparser_symbol(parser_state, lex);
      if (!karray_push_back(&rulebody, sym)) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
    } else if (lex->currtoken.kind == KEV_PTK_STR || lex->currtoken.kind == KEV_PTK_LBE) {
      KevActionFunc* actfunc = kev_pargenparser_actfunc(parser_state, lex);
      if (lex->currtoken.kind == KEV_PTK_LBE ||
          lex->currtoken.kind == KEV_PTK_STR ||
          lex->currtoken.kind == KEV_PTK_ID) {
        KlrSymbol* act_head = kev_pargenparser_symbol_create_move(parser_state, NULL, KLR_NONTERMINAL);
        if (!act_head                                                                    ||
            !kev_pargenparser_rule_create_move(parser_state, act_head, NULL, 0, actfunc) ||
            !karray_push_back(&rulebody, act_head)) {
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
  *p_bodylen = karray_size(&rulebody);
  KlrSymbol** body = (KlrSymbol**)karray_steal(&rulebody);
  karray_destroy(&rulebody);
  return body;
}

static KlrSymbol* kev_pargenparser_symbol(KevPParserState* parser_state, KevPLexer* lex) {
  char* symbol_name = lex->currtoken.attr.str;
  KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
  KlrSymbol* symbol = NULL;
  if (!node) {
    kev_error_report(lex, "undefined symbol: ", symbol_name);
    parser_state->err_count++;
    symbol = parser_state->default_symbol_nt;
  } else {
    symbol = (KlrSymbol*)node->value;
  }
  kev_pargenlexer_free_attr(lex);
  kev_pargenparser_next_nonblank(lex);
  return symbol;
}

static KlrSymbol* kev_pargenparser_symbol_kind(KevPParserState* parser_state, KevPLexer* lex, KlrSymbolKind kind) {
  char* symbol_name = lex->currtoken.attr.str;
  KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
  KlrSymbol* symbol = NULL;
  if (!node) {
    kev_error_report(lex, "undefined symbol: ", symbol_name);
    parser_state->err_count++;
    symbol = kind == KLR_TERMINAL ? parser_state->default_symbol_t : parser_state->default_symbol_nt;
  } else {
    symbol = (KlrSymbol*)node->value;
  }
  if (klr_symbol_get_kind(symbol) != kind) {
    kev_error_report(lex, "expected: ", kind == KLR_TERMINAL ? "terminal" : "non-terminal");
    parser_state->err_count++;
    symbol = kind == KLR_TERMINAL ? parser_state->default_symbol_t : parser_state->default_symbol_nt;
  }
  kev_pargenlexer_free_attr(lex);
  kev_pargenparser_next_nonblank(lex);
  return symbol;
}

static inline void kev_error_report(KevPLexer* lex, const char* info1, const char* info2) {
  kev_parser_throw_error(stderr, lex->infile, lex->filename, lex->currtoken.begin, info1, info2);
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

static KlrRule* kev_pargenparser_rule_create_move(KevPParserState* parser_state, KlrSymbol* head,
                                                  KlrSymbol** body, size_t bodylen, KevActionFunc* actfunc) {
  KlrRule* rule = klr_rule_create_move(head, body, bodylen);
  if (!rule) {
    free(body);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  if (!karray_push_back(parser_state->rules, rule)) {
    klr_rule_delete(rule);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  if (!karray_push_back(parser_state->redact, actfunc)) {
    karray_pop_back(parser_state->rules);
    klr_rule_delete(rule);
    free(actfunc->content);
    free(actfunc);
    return NULL;
  }
  klr_rule_set_id(rule, karray_size(parser_state->rules) - 1);
  return rule;
}

static KlrSymbol* kev_pargenparser_symbol_create_move(KevPParserState* parser_state, char* name, KlrSymbolKind kind) {
  KlrSymbol* symbol = klr_symbol_create_move(kind, name);
  if (!symbol) {
    free(name);
    return NULL;
  }

  if (!kev_strxmap_insert(parser_state->symbols, name, symbol)) {
    klr_symbol_delete(symbol);
    return NULL;
  }
  klr_symbol_set_id(symbol, parser_state->next_symbol_id++);
  return symbol;
}

static inline void kev_pargenparser_next_nonblank(KevPLexer* lex) {
  do {
    kev_pargenlexer_next(lex);
  } while (lex->currtoken.kind == KEV_PTK_BLKS);
}

KevActionFunc* kev_pargenparser_actfunc(KevPParserState* parser_state, KevPLexer* lex) {
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
  KlrSymbolKind symbol_kind = KLR_NONTERMINAL;
  if (kev_str_is_prefix(lex->currtoken.attr.str, "terminal")) {
    symbol_kind = KLR_TERMINAL;
  } else if (!kev_str_is_prefix(lex->currtoken.attr.str, "nonterminal") &&
             !kev_str_is_prefix(lex->currtoken.attr.str, "non-terminal")) {
    kev_error_report(lex, "unexpected: ", lex->currtoken.attr.str);
    parser_state->err_count++;
  }

  kev_pargenlexer_free_attr(lex);
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_COLON);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    KlrSymbol* symbol = kev_pargenparser_symbol_create_move(parser_state, lex->currtoken.attr.str, symbol_kind);
    if (!symbol) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
    }
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind == KEV_PTK_COLON) {
      kev_pargenparser_next_nonblank(lex);
      char* symbol_name = kev_pargenparser_expr(parser_state, lex);
      klr_symbol_set_name_move(symbol, symbol_name);
    }
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
  char* attr_name = lex->currtoken.attr.str;
  if (kev_str_is_prefix(attr_name, "priority")) {
    kev_pargenparser_statement_set_prio(parser_state, lex);
  } else if (kev_str_is_prefix(attr_name, "id")) {
    kev_pargenparser_statement_set_id(parser_state, lex);
  } else if (kev_str_is_prefix(attr_name, "start")) {
    kev_pargenparser_statement_set_start(parser_state, lex);
  } else if (kev_str_is_prefix(attr_name, "end")) {
    kev_pargenparser_statement_set_end_symbols(parser_state, lex);
  } else if (kev_str_is_prefix(attr_name, "conflict-handler")) {
    kev_pargenparser_statement_set_confhandler(parser_state, lex);
  } else if (kev_str_is_prefix(attr_name, "algorithm")) {
    kev_pargenparser_statement_set_algorithm(parser_state, lex);
  } else {
    kev_error_report(lex, "unknown attribute: ", attr_name);
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_SEMI);
    kev_pargenparser_next_nonblank(lex);
  }
  free(attr_name);
}

static void kev_pargenparser_statement_set_prio(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    /* parser_state->default_symbol is returned when the read symbol is not defined */
    KlrSymbol* symbol = kev_pargenparser_symbol(parser_state, lex);
    KlrPrioPos sympos = kev_pargenparser_sympos(parser_state, lex);
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
      if (!klr_priomap_search(parser_state->priorities, symbol, sympos)) {
        /* previously the priority of the symbol in this position is not defined */
        if (!klr_priomap_insert(parser_state->priorities, symbol, sympos, parser_state->next_priority)) {
          kev_error_report(lex, "out of memory", NULL);
            parser_state->err_count++;
        }
        if (!keep_priority)
          parser_state->next_priority++;
      } else {  /* it has been defined */
        kev_error_report(lex, "redefined priority: ", klr_symbol_get_name(symbol));
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
    KlrSymbol* symbol = kev_pargenparser_symbol(parser_state, lex);
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
  KlrSymbol* start = kev_pargenparser_symbol_kind(parser_state, lex, KLR_NONTERMINAL);
  if (parser_state->start && parser_state->start != parser_state->default_symbol_nt) {
    kev_error_report(lex, "redefine start symbol, previously defined as: ", klr_symbol_get_name(parser_state->start));
    parser_state->err_count++;
  }
  parser_state->start = start;
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static void kev_pargenparser_statement_set_end_symbols(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  while (lex->currtoken.kind == KEV_PTK_ID) {
    KlrSymbol* symbol = kev_pargenparser_symbol_kind(parser_state, lex, KLR_TERMINAL);
    if (!karray_push_back(parser_state->end_symbols, symbol)) {
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
      attribute = kev_pargenparser_expr(parser_state, lex);
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
      if (!karray_push_back(parser_state->confhandlers, handler)) {
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
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KlrPrioPos kev_pargenparser_sympos(KevPParserState* parser_state, KevPLexer* lex) {
  KlrPrioPos sympos = 1;
  if (lex->currtoken.kind == KEV_PTK_LP) {
    kev_pargenparser_next_nonblank(lex);
    if (lex->currtoken.kind == KEV_PTK_ID) {
      char* str = lex->currtoken.attr.str;
      if (kev_str_is_prefix(str, "prefix")) {
        sympos = 0;
      } else if (kev_str_is_prefix(str, "postfix")) {
        sympos = KLR_PRIOPOS_POSTFIX;
      } else {
        kev_error_report(lex, "unknown position", NULL);
        parser_state->err_count++;
      }
      free(str);
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

static void kev_pargenparser_statement_var_assign(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
  char* varname = lex->currtoken.attr.str;
  size_t symtable_id = kev_pargenparser_statement_var_assign_scope(parser_state, lex, varname);
  kev_pargenparser_next_nonblank(lex);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_ASSIGN);
  char* value = kev_pargenparser_expr(parser_state, lex);
  if (value) {
    KevSymTable* symtable = (KevSymTable*)karray_access(parser_state->symtables, symtable_id);
    if (!kev_symtable_update_move(symtable, varname, value)) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
      free(value);
    }
  }
  free(varname);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static size_t kev_pargenparser_statement_var_assign_scope(KevPParserState* parser_state, KevPLexer* lex, const char* varname) {
  if (lex->currtoken.kind == KEV_PTK_AT) {
    kev_pargenparser_next_nonblank(lex);
    char* scope = kev_pargenparser_expr(parser_state, lex);
    if (!scope)
      return 0;
    char* endptr = NULL;
    size_t scope_id = (size_t)strtoull(scope, &endptr, 10);
    if (*endptr != '\0' || scope_id >= karray_size(parser_state->symtables)) {
      kev_error_report(lex, "invalid symbol table id", scope);
      parser_state->err_count++;
      return 0;
    }
    return scope_id;
  } else {
    KevSymTableNode* node = NULL;
    size_t symtbl_id = parser_state->curr_symtbl;
    KevSymTable* symtable = NULL;
    do {
      symtable = karray_access(parser_state->symtables, symtbl_id);
      node = kev_symtable_search(symtable, varname);
      symtbl_id = kev_symtable_get_parent_id(symtable);
    } while (!node && symtbl_id != kev_symtable_get_self_id(symtable));
    return node ? kev_symtable_get_self_id(symtable) : parser_state->curr_symtbl;
  }
}

static char* kev_pargenparser_expr(KevPParserState* parser_state, KevPLexer* lex) {
  return kev_pargenparser_expr_concat(parser_state, lex);
}

static char* kev_pargenparser_expr_concat(KevPParserState* parser_state, KevPLexer* lex) {
  char* lhs = kev_pargenparser_expr_prefix(parser_state, lex);
  while (lex->currtoken.kind == KEV_PTK_CONCAT) {
    kev_pargenparser_next_nonblank(lex);
    char* rhs = kev_pargenparser_expr_prefix(parser_state, lex);
    if (!rhs || !lhs) {
      free(rhs);
      free(lhs);
      lhs = NULL;
    } else {
      char* result = kev_str_concat(lhs, rhs);
      free(lhs);
      free(rhs);
      if (!result) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
      lhs = result;
    }
  }
  return lhs;
}

static char* kev_pargenparser_expr_prefix(KevPParserState* parser_state, KevPLexer* lex) {
  return kev_pargenparser_expr_unit(parser_state, lex);
}

static char* kev_pargenparser_expr_unit(KevPParserState* parser_state, KevPLexer* lex) {
  char* ret = NULL;
  if (lex->currtoken.kind == KEV_PTK_ID) {
    KevStringMapNode* node = kev_pargenparser_symbol_search_rec(parser_state, lex->currtoken.attr.str,
                                                                parser_state->curr_symtbl);
    if (!node) {
      kev_error_report(lex, "undefined variable: ", lex->currtoken.attr.str);
      parser_state->err_count++;
      kev_pargenlexer_free_attr(lex);
      kev_pargenparser_next_nonblank(lex);
    } else {
      kev_pargenlexer_free_attr(lex);
      ret = kev_str_copy(node->value);
      if (!ret) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
      kev_pargenparser_next_nonblank(lex);
    }
  } else if (lex->currtoken.kind == KEV_PTK_IDOF) {
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_LP);
    kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
    char* symbol_name = lex->currtoken.attr.str;
    KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
    ret = NULL;
    if (!node) {
      kev_error_report(lex, "undefined symbol: ", symbol_name);
      parser_state->err_count++;
    } else {
      char buf[32];
      sprintf(buf, "%d", (int)klr_symbol_get_id((KlrSymbol*)node->value));
      if (!(ret = kev_str_copy(buf))) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
    }
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RP);
  } else if (lex->currtoken.kind == KEV_PTK_NAMEOF) {
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_LP);
    kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
    char* symbol_name = lex->currtoken.attr.str;
    KevStrXMapNode* node = kev_strxmap_search(parser_state->symbols, symbol_name);
    ret = NULL;
    if (!node) {
      kev_error_report(lex, "undefined symbol: ", symbol_name);
      parser_state->err_count++;
    } else {
      ret = kev_str_copy(klr_symbol_get_name((KlrSymbol*)node->value));
      if (!ret) {
        kev_error_report(lex, "out of memory", NULL);
        parser_state->err_count++;
      }
    }
    kev_pargenparser_next_nonblank(lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RP);
  } else if (lex->currtoken.kind == KEV_PTK_STR) {
    ret = lex->currtoken.attr.str;
    kev_pargenparser_next_nonblank(lex);
  } else if (lex->currtoken.kind == KEV_PTK_LP) {
    kev_pargenparser_next_nonblank(lex);
    ret = kev_pargenparser_expr(parser_state, lex);
    kev_pargenparser_match(parser_state, lex, KEV_PTK_RP);
  } else if (lex->currtoken.kind == KEV_PTK_THIS) {
    char buf[64];
    size_t len = (size_t)sprintf(buf, "%d", (int)parser_state->curr_symtbl);
    if (!(ret = kev_str_copy_len(buf, len))) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
    }
  } else {
    kev_error_report(lex, "unexpected: ", kev_pargenlexer_info(lex->currtoken.kind));
    parser_state->err_count++;
    kev_pargenparser_recover(lex, KEV_PTK_ID);
    ret = kev_pargenparser_expr_unit(parser_state, lex);
  }
  return kev_pargenparser_expr_postfix(parser_state, lex, ret);
}

static char* kev_pargenparser_expr_postfix(KevPParserState* parser_state, KevPLexer* lex, char* value) {
  char* ret = value;
  while (true) {
    if (lex->currtoken.kind == KEV_PTK_COLON) {
      if (!ret) {
        kev_pargenparser_next_nonblank(lex);
        kev_pargenparser_match(parser_state, lex, KEV_PTK_ID);
        continue;
      }
      char* endptr = NULL;
      size_t symtbl_id = (size_t)strtoull(value, &endptr, 10);
      if (*endptr != '\0' || symtbl_id >= karray_size(parser_state->symtables)) {
        kev_error_report(lex, "not a valid symbol table id: ", ret);
        parser_state->err_count++;
        free(ret);
        ret = NULL;
        kev_pargenparser_next_nonblank(lex);
        kev_pargenparser_match(parser_state, lex, KEV_PTK_ID);
        continue;
      }
      free(ret);
      kev_pargenparser_next_nonblank(lex);
      kev_pargenparser_guarantee(parser_state, lex, KEV_PTK_ID);
      KevSymTable* symtable = (KevSymTable*)karray_access(parser_state->symtables, symtbl_id);
      KevSymTableNode* node = kev_symtable_search(symtable, lex->currtoken.attr.str);
      if (!node) {
        kev_error_report(lex, "undefined variable: ", lex->currtoken.attr.str);
        parser_state->err_count++;
        ret = NULL;
      } else {
        ret = kev_str_copy(node->value);
      }
      kev_pargenparser_next_nonblank(lex);
    } else {
      break;
    }
  }
  return ret;
}

static void kev_pargenparser_statement_import(KevPParserState* parser_state, KevPLexer* lex) {
  kev_pargenparser_next_nonblank(lex);
  char* import_path = kev_pargenparser_expr(parser_state, lex);
  if (kev_is_relative(import_path)) {
    KevSymTable* curr_symtbl = (KevSymTable*)karray_access(parser_state->symtables, parser_state->curr_symtbl);
    KevStringMapNode* node = kev_symtable_search(curr_symtbl, "@file-directory");
    char* base = node ? node->value : "";
    char* absolute_path = NULL;
    if (!(absolute_path = kev_str_concat(base, import_path))) {
      kev_error_report(lex, "out of memory", NULL);
      parser_state->err_count++;
    }
    free(import_path);
    import_path = absolute_path;
  }
  if (import_path) {
    if (!kev_pargenparser_parse_file(parser_state, import_path)) {
      kev_error_report(lex, "can not open file: ", import_path);
      parser_state->err_count++;
    }
  }
  free(import_path);
  kev_pargenparser_match(parser_state, lex, KEV_PTK_SEMI);
}

static KevSymTableNode* kev_pargenparser_symbol_search_rec(KevPParserState* parser_state, const char* key, size_t begin_symtbl_id) {
  KevSymTableNode* node = NULL;
  size_t symtbl_id = begin_symtbl_id;
  KevSymTable* symtable = NULL;
  do {
    symtable = karray_access(parser_state->symtables, symtbl_id);
    node = kev_symtable_search(symtable, key);
    symtbl_id = kev_symtable_get_parent_id(symtable);
  } while (!node && symtbl_id != kev_symtable_get_self_id(symtable));
  return node;
}
