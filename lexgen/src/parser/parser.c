#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/parser/parser.h"
#include "lexgen/include/parser/error.h"
#include "lexgen/include/parser/regex.h"
#include "utils/include/os_spec/dir.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>

static int kev_lexgenparser_next_nonblank(KevLLexer* lex, KevLToken* token);
static int kev_lexgenparser_match(KevLLexer* lex, KevLToken* token, int kind);
static int kev_lexgenparser_guarantee(KevLLexer* lex, KevLToken* token, int kind);
static int kev_lexgenparser_proc_func_name(KevLLexer* lex, KevLToken* token, char** p_name);
static int kev_lexgenparser_set_pattern_attribute(KevLLexer* lex, KevLToken* token, KevAddrArray* macros, size_t* p_pattern_id);
static char* kev_get_id_name(char* id);

bool kev_lexgenparser_init(KevLParserState* parser_state) {
  if (!parser_state) return false;
  if (!kev_patternlist_init(&parser_state->list))
    return false;
  if (!kev_strfamap_init(&parser_state->nfa_map, 8)) {
    kev_patternlist_destroy(&parser_state->list);
    return false;
  }
  if (!kev_strmap_init(&parser_state->env_var, 8)) {
    kev_patternlist_destroy(&parser_state->list);
    kev_strfamap_destroy(&parser_state->nfa_map);
    return false;
  }
  return true;
}

void kev_lexgenparser_destroy(KevLParserState* parser_state) {
  kev_patternlist_free_content(&parser_state->list);
  kev_patternlist_destroy(&parser_state->list);
  kev_strfamap_destroy(&parser_state->nfa_map);
  kev_strmap_destroy(&parser_state->env_var);
}

int kev_lexgenparser_statement_nfa_assign(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state) {
  KevPatternList* list = &parser_state->list;
  KevStringFaMap* nfa_map = &parser_state->nfa_map;
  int err_count = 0;
  char* name = kev_get_id_name(token->attr);
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ASSIGN);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
  KevFA* nfa = kev_regex_parse((uint8_t*)token->attr + 1, nfa_map);
  if (!nfa) {
    kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos() + 1);
    free(name);
    err_count++;
  } else {
    if (!kev_pattern_insert(list->tail, name, nfa)) {
      kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
      err_count++;
      free(name);
      kev_fa_delete(nfa);
    }
    if (!kev_strfamap_update(nfa_map, name, nfa)) {
      kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
      /* name and nfa are stored in list, so there is no need to free them here */
      err_count++;
    }
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  return err_count;
}

int kev_lexgenparser_statement_deftoken(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state) {
  KevPatternList* list = &parser_state->list;
  KevStringFaMap* nfa_map = &parser_state->nfa_map;
  int err_count = 0;
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
  char* name = kev_get_id_name(token->attr);
  KevAddrArray* macros = kev_addrarray_create();
  if (!name || !macros) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
    free(name);
    kev_addrarray_delete(macros);
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  size_t pattern_id = parser_state->list.pattern_no;
  err_count += kev_lexgenparser_set_pattern_attribute(lex, token, macros, &pattern_id);
  if (!kev_patternlist_insert(list, name, macros, pattern_id)) {
    kev_parser_error_report(stderr, lex->infile, "failed to add new pattern", token->begin);
    err_count++;
  }
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_COLON);
  do {
    char* proc_name = NULL;
    err_count += kev_lexgenparser_proc_func_name(lex, token, &proc_name);
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
    uint8_t* regex = (uint8_t*)token->attr + 1;
    KevFA* nfa = kev_regex_parse(regex, nfa_map);
    if (!nfa) {
      kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos() + 1);
      free(proc_name);
      err_count++;
    } else {
      if (!kev_pattern_insert(list->tail, proc_name, nfa)) {
        kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
        err_count++;
      }
    }
    err_count += kev_lexgenparser_next_nonblank(lex, token);
  } while (token->kind == KEV_LEXGEN_TOKEN_REGEX || token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN);
  return err_count;
}

int kev_lexgenparser_statement_env_var_def(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state) {
  int err_count = 0;
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
  char* env_var = kev_get_id_name(token->attr);
  if (!env_var) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ASSIGN);
  if (token->kind != KEV_LEXGEN_TOKEN_LONG_STR && token->kind != KEV_LEXGEN_TOKEN_STR) {
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_STR);
    free(env_var);
    return err_count;
  }
  if (!env_var) {
    return err_count + kev_lexgenparser_next_nonblank(lex, token);
  } else {
    char* env_value = NULL;
    if (token->kind == KEV_LEXGEN_TOKEN_LONG_STR) {
      size_t len = token->end - token->begin;
      token->attr[len - 2] = '\0';  /* long string ends with two '\n', eliminate them */
    }
    env_value = token->attr + 1;
    if (!kev_strmap_update(&parser_state->env_var, env_var, env_value)) {
      kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
      err_count++;
    }
    free(env_var);
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    return err_count;
  }
}

int kev_lexgenparser_parse(char* filepath, KevLParserState* parser_state) {
  int err_count = 0;
  FILE* input = fopen(filepath, "r");
  if (!input) {
    fprintf(stderr, "error: faied to open file: %s\n", filepath);
    return err_count + 1;
  }
  KevLLexer lex;
  if (!kev_lexgenlexer_init(&lex, input)) {
    kev_parser_error_report(stderr, input, "failed to initialize lexer", 0);
    fclose(input);
    return err_count + 1;
  }

  if (!kev_strmap_update(&parser_state->env_var, "import-path", kev_trunc_leaf(filepath))) {
    fclose(input);
    kev_lexgenlexer_destroy(&lex);
    kev_parser_error_report(stderr, lex.infile, "failed to initialize import-path", 0);
    return err_count + 1;
  }

  KevLToken token;
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  err_count += kev_lexgenparser_lex_src(&lex, &token, parser_state);

  kev_lexgenlexer_destroy(&lex);
  fclose(input);
  return err_count;
}

int kev_lexgenparser_statement_import(KevLLexer* lex, KevLToken* token, KevLParserState* parser_state) {
  int err_count = 0;
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_STR);

  char* relpath = token->attr + 1;
  char* base = NULL;
  KevStringMapNode* node = kev_strmap_search(&parser_state->env_var, "import-path");
  char* path = NULL;
  if (node)
    base = kev_str_copy(node->value);
  else
    base = kev_str_copy("./");
  if (!base || !(path = kev_str_concat(base, relpath))) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    free(base);
    return err_count + 1;
  }

  err_count += kev_lexgenparser_parse(path, parser_state);
  if (!kev_strmap_update(&parser_state->env_var, "import-path", base)) {
    err_count++;
    kev_parser_error_report(stderr, lex->infile, "failed to recovery base path", token->begin);
  };
  free(base);
  free(path);
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  return err_count;
}

int kev_lexgenparser_lex_src(KevLLexer *lex, KevLToken *token, KevLParserState* parser_state) {
  int err_count = 0;
  while (token->kind == KEV_LEXGEN_TOKEN_BLANKS) 
    err_count += kev_lexgenparser_next_nonblank(lex, token);
  do {
    if (token->kind == KEV_LEXGEN_TOKEN_ID) {
      err_count += kev_lexgenparser_statement_nfa_assign(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_DEF) {
      err_count += kev_lexgenparser_statement_deftoken(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_ENV_VAR_DEF) {
      err_count += kev_lexgenparser_statement_env_var_def(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_IMPORT) {
      err_count += kev_lexgenparser_statement_import(lex, token, parser_state);
    }
    else {
      kev_parser_error_report(stderr, lex->infile, "expected \'def\' or identifer", token->begin);
      kev_parser_error_handling(stderr, lex, KEV_LEXGEN_TOKEN_DEF, false);
      err_count++;
    }
  } while (token->kind != KEV_LEXGEN_TOKEN_END);
  return err_count;
}

static int kev_lexgenparser_next_nonblank(KevLLexer* lex, KevLToken* token) {
  int err_count = 0;
  do {
    if (!kev_lexgenlexer_next(lex, token)) {
      kev_parser_error_report(stderr, lex->infile, "lexical error", token->begin);
      kev_parser_error_handling(stderr, lex, KEV_LEXGEN_TOKEN_BLANKS, true);
      err_count++;
    }
  } while (token->kind == KEV_LEXGEN_TOKEN_BLANKS);
  return err_count;
}

static int kev_lexgenparser_match(KevLLexer* lex, KevLToken* token, int kind) {
  int err_count = 0;
  if (token->kind != kind) {
    static char err_info_buf[1024];
    sprintf(err_info_buf, "expected %s", kev_lexgenlexer_info(kind));
    kev_parser_error_report(stderr, lex->infile, err_info_buf, token->begin);
    kev_parser_error_handling(stderr, lex, kind, false);
    err_count = 1;
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  return err_count;
}

static int kev_lexgenparser_guarantee(KevLLexer* lex, KevLToken* token, int kind) {
  if (token->kind != kind) {
    static char err_info_buf[1024];
    sprintf(err_info_buf, "expected %s", kev_lexgenlexer_info(kind));
    kev_parser_error_report(stderr, lex->infile, err_info_buf, token->begin);
    kev_parser_error_handling(stderr, lex, kind, false);
    return 1;
  }
  return 0;
}

static int kev_lexgenparser_proc_func_name(KevLLexer* lex, KevLToken* token, char** p_name) {
  int err_count = 0;
  if (token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN) {
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
    char* name = kev_get_id_name(token->attr);
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
    *p_name = name;
  } else {
    *p_name = NULL;
  }
  return err_count;
}

static int kev_lexgenparser_set_pattern_attribute(KevLLexer* lex, KevLToken* token, KevAddrArray* macros, size_t* p_pattern_id) {
  int err_count = 0;
  if (token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN) {
    while (true) {
      err_count += kev_lexgenparser_next_nonblank(lex, token);
      if (token->kind == KEV_LEXGEN_TOKEN_ID) {
        char* name = kev_get_id_name(token->attr);
        if (!name || !kev_addrarray_push_back(macros, name)) {
          kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
          free(name);
          err_count++;
        }
      } else if (token->kind == KEV_LEXGEN_TOKEN_NUMBER) {
        *p_pattern_id = (size_t)strtoull(token->attr, NULL, 10);
      } else {
        err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
        break;
      }
      err_count += kev_lexgenparser_next_nonblank(lex, token);
      if (token->kind != KEV_LEXGEN_TOKEN_COMMA)
        break;
    }
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
  }
  return err_count;
}

static char* kev_get_id_name(char* id) {
  char* name = NULL;
  if (id[0] == '\'') {
    name = (char*)malloc(sizeof (char) * (kev_str_len(id) - 1));
    size_t i = 0;
    if (!name) return NULL;
    while (*++id != '\'') {
      if (*id == '\\') {
        switch (*++id) {
          case 'n': name[i++] = '\n'; break;
          case 'r': name[i++] = '\r'; break;
          case 't': name[i++] = '\t'; break;
          case '\'': name[i++] = '\''; break;
          case '\"': name[i++] = '\"'; break;
          case '\\': name[i++] = '\\'; break;
          default: name[i++] = *id; break;
        }
      } else {
        name[i++] = *id;
      }
    }
    name[i] = '\0';
  } else {
    name = kev_str_copy(id);
  }
  return name;
}
