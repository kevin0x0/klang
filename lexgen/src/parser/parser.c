#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/parser/parser.h"
#include "lexgen/include/parser/error.h"
#include "lexgen/include/parser/regex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* token_description[] = {
  [KEV_LEXGEN_TOKEN_ASSIGN] = "\'=\'", [KEV_LEXGEN_TOKEN_END] = "EOF",
  [KEV_LEXGEN_TOKEN_BLANKS] = "blanks", [KEV_LEXGEN_TOKEN_COLON] = "\':\'",
  [KEV_LEXGEN_TOKEN_ID] = "identifier", [KEV_LEXGEN_TOKEN_REGEX] = "regular expression",
  [KEV_LEXGEN_TOKEN_OPEN_PAREN] = "\'(\'", [KEV_LEXGEN_TOKEN_CLOSE_PAREN] = "\')\'",
  [KEV_LEXGEN_TOKEN_DEF] = "def", [KEV_LEXGEN_TOKEN_TMPL_ID] = "template identifier",
};

static int kev_lexgenparser_next_nonblank(KevLexGenLexer* lex, KevLexGenToken* token);
static int kev_lexgenparser_match(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static int kev_lexgenparser_guarantee(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static int kev_lexgenparser_proc_func_name(KevLexGenLexer* lex, KevLexGenToken* token, char** p_name);
static inline bool kev_char_range(KevFA* nfa, int64_t begin, int64_t end);
static char* kev_get_string(char* str);

bool kev_lexgenparser_init(KevParserState* parser_state) {
  if (!parser_state) return false;
  if (!kev_patternlist_init(&parser_state->list))
    return false;
  if (!kev_strfamap_init(&parser_state->nfa_map, 8)) {
    kev_patternlist_destroy(&parser_state->list);
    return false;
  }
  if (!kev_strmap_init(&parser_state->tmpl_map, 8)) {
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
  kev_strmap_destroy(&parser_state->tmpl_map);
}

int kev_lexgenparser_statement_assign(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
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
  KevFA* nfa = kev_regex_parse((uint8_t*)token->attr, nfa_map);
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
  size_t len = strlen(token->attr);
  char* name = (char*)malloc(sizeof(char) * (len + 1));
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  } else {
    strcpy(name, token->attr);
  }
  if (!kev_patternlist_insert(list, name)) {
    kev_parser_error_report(stderr, lex->infile, "failed to add new pattern", token->begin);
    err_count++;
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_COLON);
  do {
    char* proc_name = NULL;
    err_count += kev_lexgenparser_proc_func_name(lex, token, &proc_name);
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
    uint8_t* regex = (uint8_t*)token->attr;
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

int kev_lexgenparser_statement_tmpl_def(KevLexGenLexer* lex, KevLexGenToken* token, KevParserState* parser_state) {
  int err_count = 0;
  size_t len = strlen(token->attr + 1);
  char* tmpl_name = (char*)malloc(sizeof(char) * (len + 1));
  if (!tmpl_name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
    err_count++;
  } else {
    strcpy(tmpl_name, token->attr + 1);
  }
  err_count += kev_lexgenparser_next_nonblank(lex, token);
  err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ASSIGN);
  if (!tmpl_name) {
    err_count += kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ID);
    return err_count;
  } else {
    err_count += kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_ID);
    char* name = token->attr;
    if (!kev_strmap_update(&parser_state->tmpl_map, tmpl_name, name)) {
      kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
      err_count++;
    }
    free(tmpl_name);
    err_count += kev_lexgenparser_next_nonblank(lex, token);
    return err_count;
  }
}

int kev_lexgenparser_lex_src(KevLexGenLexer *lex, KevLexGenToken *token, KevParserState* parser_state) {
  int err_count = 0;
  do {
    if (token->kind == KEV_LEXGEN_TOKEN_ID) {
      err_count += kev_lexgenparser_statement_assign(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_DEF) {
      err_count += kev_lexgenparser_statement_deftoken(lex, token, parser_state);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_TMPL_ID) {
      err_count += kev_lexgenparser_statement_tmpl_def(lex, token, parser_state);
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
    sprintf(err_info_buf, "expected %s", token_description[kind]);
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
    sprintf(err_info_buf, "expected %s", token_description[kind]);
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

bool kev_lexgenparser_posix_charset(KevParserState* parser_state) {
  KevPatternList* list = &parser_state->list;
  KevStringFaMap* nfa_map = &parser_state->nfa_map;
  KevFA* nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  char* name = kev_get_string("print");
  if (!kev_char_range(nfa, 32, 127) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("graph");
  if (!kev_char_range(nfa, 33, 127) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("alnum");
  if (!kev_char_range(nfa, 'a', 'z' + 1) ||
      !kev_char_range(nfa, 'A', 'Z' + 1) ||
      !kev_char_range(nfa, '0', '9' + 1) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("alpha");
  if (!kev_char_range(nfa, 'a', 'z' + 1) ||
      !kev_char_range(nfa, 'A', 'Z' + 1) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("digit");
  if (!kev_char_range(nfa, '0', '9' + 1) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("lower");
  if (!kev_char_range(nfa, 'a', 'z' + 1) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  nfa = kev_nfa_create(KEV_NFA_SYMBOL_EMPTY);
  name = kev_get_string("upper");
  if (!kev_char_range(nfa, 'A', 'Z' + 1) ||
      !kev_pattern_insert(list->head, name, nfa)) {
    kev_fa_delete(nfa);
    free(name);
    return false;
  }
  if (!kev_strfamap_update(nfa_map, name, nfa)) {
    return false;
  }

  return true;
}

static inline bool kev_char_range(KevFA* nfa, int64_t begin, int64_t end) {
  if (begin < 0 || end < 0) return false;
  for (int64_t c = begin; c < end; ++c) {
    if (!kev_nfa_add_transition(nfa, c)) {
      return false;
    }
  }
  return true;
}

static char* kev_get_string(char* str) {
  char* ret = (char*)malloc(sizeof (char) * (strlen(str) + 1));
  if (!ret) return NULL;
  strcpy(ret, str);
  return ret;
}
