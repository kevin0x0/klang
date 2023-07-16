#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/parser/parser.h"
#include "lexgen/include/parser/error.h"
#include "lexgen/include/parser/regex.h"
#include "utils/include/dir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int kev_lexgenparser_next_nonblank(KevLexGenLexer* lex, KevLexGenToken* token);
static int kev_lexgenparser_match(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static int kev_lexgenparser_guarantee(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static int kev_lexgenparser_proc_func_name(KevLexGenLexer* lex, KevLexGenToken* token, char** p_name);
static int kev_lexgenparser_macro_name(KevLexGenLexer* lex, KevLexGenToken* token, char** p_name);
static char* kev_copy_string(char* str);

bool kev_lexgenparser_init(KevParserState* parser_state) {
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

void kev_lexgenparser_destroy(KevParserState* parser_state) {
  kev_patternlist_free_content(&parser_state->list);
  kev_patternlist_destroy(&parser_state->list);
  kev_strfamap_destroy(&parser_state->nfa_map);
  kev_strmap_destroy(&parser_state->env_var);
}

int kev_lexgenparser_statement_nfa_assign(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
  KevPatternList* list = &parser_state->list;
  KevStringFaMap* nfa_map = &parser_state->nfa_map;
  int err_count = 0;
  size_t len = strlen(token->attr);
  char* name = (char*)malloc(sizeof(char) * (len + 1));
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  } else {
    strcpy(name, token->attr);
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ASSIGN);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
  KevFA* nfa = kev_regex_parse((uint8_t*)token->attr + 1, nfa_map);
  if (!nfa) {
    kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos());
    free(name);
    err_count++;
  } else {
    if (!kev_pattern_insert(list->tail, name, nfa) ||
        !kev_strfamap_update(nfa_map, name, nfa)) {
      kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
      err_count++;
    }
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  return err_count;
}

int kev_lexgenparser_statement_deftoken(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
  KevPatternList* list = &parser_state->list;
  KevStringFaMap* nfa_map = &parser_state->nfa_map;
  int err_count = 0;
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
  char* name = kev_copy_string(token->attr);
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  char* macro = NULL;
  err_count += kev_lexgenparser_macro_name(lex, token, &macro);
  if (!kev_patternlist_insert(list, name, macro)) {
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
      kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos());
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

int kev_lexgenparser_statement_env_var_assgn(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
  int err_count = 0;
  size_t len = strlen(token->attr + 1);
  char* env_var = (char*)malloc(sizeof(char) * (len + 1));
  if (!env_var) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  } else {
    strcpy(env_var, token->attr + 1);
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

int kev_lexgenparser_parse(char* filepath, KevParserState* parser_state) {
  int err_count = 0;
  FILE* input = fopen(filepath, "r");
  if (!input) {
    fprintf(stderr, "error: faied to open file: %s\n", filepath);
    return err_count + 1;
  }
  KevLexGenLexer lex;
  if (!kev_lexgenlexer_init(&lex, input)) {
    fclose(input);
    kev_parser_error_report(stderr, input, "failed to initialize lexer", 0);
    return err_count + 1;
  }

  if (!kev_strmap_update(&parser_state->env_var, "import-path", kev_trunc_leaf(filepath))) {
    fclose(input);
    kev_lexgenlexer_destroy(&lex);
    kev_parser_error_report(stderr, lex.infile, "failed to initialize import-path", 0);
    return err_count + 1;
  }

  KevLexGenToken token;
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  err_count += kev_lexgenparser_lex_src(&lex, &token, parser_state);

  kev_lexgenlexer_destroy(&lex);
  fclose(input);
  return err_count;
}

int kev_lexgenparser_statement_import(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
  int err_count = 0;
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_STR);

  char* relpath = kev_copy_string(token->attr + 1);
  if (!relpath) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    return err_count + 1;
  }
  char* base = NULL;
  KevStringMapNode* node = kev_strmap_search(&parser_state->env_var, "import-path");
  if (node)
    base = kev_copy_string(node->value);
  else
    base = kev_copy_string("./");
  char* path = (char*)malloc(sizeof (char) * (strlen(base) + strlen(relpath) + 1));
  if (!path) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    free(base);
    free(relpath);
    return err_count + 1;
  }
  strcpy(path, base);
  strcat(path, relpath);

  err_count += kev_lexgenparser_parse(path, parser_state);
  if (!kev_strmap_update(&parser_state->env_var, "import-path", base)) {
    err_count++;
    kev_parser_error_report(stderr, lex->infile, "failed to recovery base path", token->begin);
  };
  free(base);
  free(relpath);
  free(path);
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  return err_count;
}

int kev_lexgenparser_lex_src(KevLexGenLexer *lex, KevLexGenToken *token, KevParserState* parser_state) {
  int err_count = 0;
  do {
    if (token->kind == KEV_LEXGEN_TOKEN_ID) {
      err_count += kev_lexgenparser_statement_nfa_assign(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_DEF) {
      err_count += kev_lexgenparser_statement_deftoken(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_ENV_VAR) {
      err_count += kev_lexgenparser_statement_env_var_assgn(lex, token, parser_state);
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

static int kev_lexgenparser_next_nonblank(KevLexGenLexer* lex, KevLexGenToken* token) {
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

static int kev_lexgenparser_match(KevLexGenLexer* lex, KevLexGenToken* token, int kind) {
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

static int kev_lexgenparser_guarantee(KevLexGenLexer* lex, KevLexGenToken* token, int kind) {
  if (token->kind != kind) {
    static char err_info_buf[1024];
    sprintf(err_info_buf, "expected %s", kev_lexgenlexer_info(kind));
    kev_parser_error_report(stderr, lex->infile, err_info_buf, token->begin);
    kev_parser_error_handling(stderr, lex, kind, false);
    return 1;
  }
  return 0;
}

static int kev_lexgenparser_proc_func_name(KevLexGenLexer* lex, KevLexGenToken* token, char** p_name) {
  int err_count = 0;
  if (token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN) {
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
    size_t len = strlen(token->attr);
    char* name = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(name, token->attr);
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
    *p_name = name;
  } else {
    *p_name = NULL;
  }
  return err_count;
}

static int kev_lexgenparser_macro_name(KevLexGenLexer* lex, KevLexGenToken* token, char** p_name) {
  int err_count = 0;
  if (token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN) {
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
    size_t len = strlen(token->attr);
    char* name = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(name, token->attr);
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
    *p_name = name;
  } else {
    *p_name = NULL;
  }
  return err_count;
}

static char* kev_copy_string(char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) return NULL;
  strcpy(ret, str);
  return ret;
}
