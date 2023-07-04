#include "lexgen/include/parser/parser.h"
#include "lexgen/include/parser/error.h"
#include "lexgen/include/parser/regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* token_description[] = {
  [KEV_LEXGEN_TOKEN_ASSIGN] = "\'=\'", [KEV_LEXGEN_TOKEN_END] = "EOF",
  [KEV_LEXGEN_TOKEN_BLANKS] = "blanks", [KEV_LEXGEN_TOKEN_COLON] = "\':\'",
  [KEV_LEXGEN_TOKEN_NAME] = "identifier", [KEV_LEXGEN_TOKEN_REGEX] = "regular expression",
  [KEV_LEXGEN_TOKEN_OPEN_PAREN] = "\'(\'", [KEV_LEXGEN_TOKEN_CLOSE_PAREN] = "\')\'",
  [KEV_LEXGEN_TOKEN_DEF] = "def",
};

static void kev_lexgenparser_next_nonblank(KevLexGenLexer* lex, KevLexGenToken* token);
static void kev_lexgenparser_match(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static void kev_lexgenparser_guarantee(KevLexGenLexer* lex, KevLexGenToken* token, int kind);
static char* kev_lexgenparser_proc_func_name(KevLexGenLexer* lex, KevLexGenToken* token);

void kev_lexgenparser_statement_assign(KevLexGenLexer* lex, KevLexGenToken* token, KevPatternList* list, KevStringFaMap* nfa_map) {
  size_t len = strlen(token->attr);
  char* name = (char*)malloc(sizeof(char) * (len + 1));
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
  } else {
    strcpy(name, token->attr);
  }
  kev_lexgenparser_next_nonblank(lex, token);
  kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_ASSIGN);
  kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
  KevFA* nfa = kev_regex_parse((uint8_t*)token->attr, nfa_map);
  if (!nfa) {
    kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos());
    free(name);
  } else {
    if (!kev_pattern_insert(list->tail, name, nfa) ||
        !kev_strfamap_update(nfa_map, name, nfa)) {
      kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
    }
  }
  kev_lexgenparser_next_nonblank(lex, token);
}

void kev_lexgenparser_statement_deftoken(KevLexGenLexer* lex, KevLexGenToken* token, KevPatternList* list, KevStringFaMap* nfa_map) {
  kev_lexgenparser_next_nonblank(lex, token);
  kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_NAME);
  size_t len = strlen(token->attr);
  char* name = (char*)malloc(sizeof(char) * (len + 1));
  if (!name) {
    kev_parser_error_report(stderr, lex->infile, "out of memory", token->begin);
  } else {
    strcpy(name, token->attr);
  }
  if (!kev_patternlist_insert(list, name)) {
    kev_parser_error_report(stderr, lex->infile, "failed to add new pattern", token->begin);
  }
  kev_lexgenparser_next_nonblank(lex, token);
  kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_COLON);
  do {
    char* proc_name = kev_lexgenparser_proc_func_name(lex, token);
    kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_REGEX);
    uint8_t* regex = (uint8_t*)token->attr;
    KevFA* nfa = kev_regex_parse(regex, nfa_map);
    if (!nfa) {
      kev_parser_error_report(stderr, lex->infile, kev_regex_get_info(), token->begin + kev_regex_get_pos());
      free(proc_name);
    } else {
      if (!kev_pattern_insert(list->tail, proc_name, nfa)) {
        kev_parser_error_report(stderr, lex->infile, "failed to register NFA", token->begin);
      }
    }
    kev_lexgenparser_next_nonblank(lex, token);
  } while (token->kind == KEV_LEXGEN_TOKEN_REGEX || token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN);
}

void kev_lexgenparser_lex_src(KevLexGenLexer *lex, KevLexGenToken *token, KevPatternList *list, KevStringFaMap *nfa_map) {
  do {
    if (token->kind == KEV_LEXGEN_TOKEN_NAME) {
      kev_lexgenparser_statement_assign(lex, token, list, nfa_map);
    }
    else if (token->kind == KEV_LEXGEN_TOKEN_DEF) {
      kev_lexgenparser_statement_deftoken(lex, token, list, nfa_map);
    }
    else {
      kev_parser_error_report(stderr, lex->infile, "expected \'def\' or identifer", token->begin);
      kev_parser_error_handling(stderr, lex, KEV_LEXGEN_TOKEN_DEF, false);
    }
  } while (token->kind != KEV_LEXGEN_TOKEN_END);
}

static void kev_lexgenparser_next_nonblank(KevLexGenLexer* lex, KevLexGenToken* token) {
  do {
    if (!kev_lexgenlexer_next(lex, token)) {
      kev_parser_error_report(stderr, lex->infile, "syntax error", token->begin);
      kev_parser_error_handling(stderr, lex, KEV_LEXGEN_TOKEN_BLANKS, true);
    }
  } while (token->kind == KEV_LEXGEN_TOKEN_BLANKS);
}

static void kev_lexgenparser_match(KevLexGenLexer* lex, KevLexGenToken* token, int kind) {
  if (token->kind != kind) {
    static char err_info_buf[1024];
    sprintf(err_info_buf, "expected %s", token_description[kind]);
    kev_parser_error_report(stderr, lex->infile, err_info_buf, token->begin);
    kev_parser_error_handling(stderr, lex, kind, false);
  }
  kev_lexgenparser_next_nonblank(lex, token);
}

static void kev_lexgenparser_guarantee(KevLexGenLexer* lex, KevLexGenToken* token, int kind) {
  if (token->kind != kind) {
    static char err_info_buf[1024];
    sprintf(err_info_buf, "expected %s", token_description[kind]);
    kev_parser_error_report(stderr, lex->infile, err_info_buf, token->begin);
    kev_parser_error_handling(stderr, lex, kind, false);
  }
}

static char* kev_lexgenparser_proc_func_name(KevLexGenLexer* lex, KevLexGenToken* token) {
  if (token->kind == KEV_LEXGEN_TOKEN_OPEN_PAREN) {
    kev_lexgenparser_next_nonblank(lex, token);
    kev_lexgenparser_guarantee(lex, token, KEV_LEXGEN_TOKEN_NAME);
    size_t len = strlen(token->attr);
    char* name = (char*)malloc(sizeof(char) * (len + 1));
    strcpy(name, token->attr);
    kev_lexgenparser_next_nonblank(lex, token);
    kev_lexgenparser_match(lex, token, KEV_LEXGEN_TOKEN_CLOSE_PAREN);
    return name;
  }
  return NULL;
}
