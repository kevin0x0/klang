#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/parser/parser.h"
#include "lexgen/include/lexgen/output.h"

#include <stdlib.h>

static void fatal_error(char* info, char* info2);

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state);
/* return the number of NFAs */
static size_t kev_lexgen_control_generate(KevPatternList* list, KevStringFaMap* map, KevFA** p_min_dfa, size_t** p_acc_mapping);
static void kev_lexgen_control_regularize_mapping(KevFA* dfa, size_t nfa_no, KevPatternList* list, size_t* acc_mapping, size_t** p_pattern_mapping);
/* return NULL-terminated function name array */
static char** kev_lexgen_construct_callback_array(KevFA* dfa, KevPatternList* list, size_t* acc_mapping, size_t nfa_no, size_t* p_arrlen);

void kev_lexgen_control(KevOptions* options) {
  /* show help */
  if (options->opts[KEV_LEXGEN_OPT_HELP]) {
    kev_lexgen_output_help();
    return;
  }

  KevParserState parser_state;
  if (!kev_lexgenparser_init(&parser_state)) {
    fatal_error("failed to initialize parser", NULL);
  }
  /* read lexical description file */
  FILE* input = fopen(options->strs[KEV_LEXGEN_INPUT_PATH], "r");
  if (!input) {
    kev_lexgenparser_destroy(&parser_state);
    fatal_error("can not open file:", options->strs[KEV_LEXGEN_INPUT_PATH]);
  }
  int error_number = kev_lexgen_control_parse(input, &parser_state);
  if (error_number != 0) {
    fprintf(stderr, "%d error(s) detected.\n", error_number);
    exit(EXIT_FAILURE);
  }
  fclose(input);
  /* generate minimized DFA */
  KevFA* dfa;
  size_t* acc_mapping;
  size_t nfa_no = kev_lexgen_control_generate(&parser_state.list, &parser_state.nfa_map, &dfa, &acc_mapping);
  /* output transition table */
  FILE* output_table = fopen(options->strs[KEV_LEXGEN_OUT_TAB_PATH], "w");
  if (!output_table) {
    fatal_error("can not open file: ", options->strs[KEV_LEXGEN_OUT_TAB_PATH]);
  }
  kev_lexgen_output_table(output_table, dfa, acc_mapping, options);
  /* output optional infomation */
  size_t* pattern_mapping = NULL;
  kev_lexgen_control_regularize_mapping(dfa, nfa_no, &parser_state.list, acc_mapping, &pattern_mapping);
  kev_lexgen_output_info(output_table, &parser_state.list, pattern_mapping, options);
  free(pattern_mapping);
  fclose(output_table);
  /* output source if specified */
  if (options->opts[KEV_LEXGEN_OPT_STAGE] >= KEV_LEXGEN_OPT_STA_SRC) {
    size_t arrlen = 0;
    char** callbacks = kev_lexgen_construct_callback_array(dfa, &parser_state.list, acc_mapping, nfa_no, &arrlen);
    kev_lexgen_output_src(options, &parser_state.tmpl_map, callbacks, arrlen);
    free(callbacks);
  }
  free(acc_mapping);
  kev_fa_delete(dfa);
  /* compile source to archive file or shared object file if specified */
  if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_ARC) {
    kev_lexgen_output_arc(options->strs[KEV_LEXGEN_BUILD_TOOL_NAME], options);
  } else if (options->opts[KEV_LEXGEN_OPT_STAGE] == KEV_LEXGEN_OPT_STA_SHA) {
    kev_lexgen_output_sha(options->strs[KEV_LEXGEN_BUILD_TOOL_NAME], options);
  }
  /* free resources */
  kev_lexgenparser_destroy(&parser_state);
}

static void fatal_error(char* info, char* info2) {
  fputs("fatal: ", stderr);
  if (info)
    fputs(info, stderr);
  if (info2)
    fputs(info, stderr);
  fputs("\nterminated\n", stderr);
  exit(EXIT_FAILURE);
}

static int kev_lexgen_control_parse(FILE* input, KevParserState* parser_state) {
  KevLexGenLexer lex;
  KevLexGenToken token;
  if (!kev_lexgenlexer_init(&lex, input))
    fatal_error("kev_lexgenlexer_init() failed", NULL);
  if (!kev_lexgenparser_posix_charset(parser_state))
    fatal_error("failed to initialize POSIX charset", NULL);
  while (!kev_lexgenlexer_next(&lex, &token))
    continue;
  int error_number = kev_lexgenparser_lex_src(&lex, &token, parser_state);
  kev_lexgenlexer_destroy(&lex);
  return error_number;
}

static size_t kev_lexgen_control_generate(KevPatternList* list, KevStringFaMap* map, KevFA** p_min_dfa, size_t** p_acc_mapping) {
  size_t nfa_no = 0;
  KevPattern* pattern = list->head->next; /* fisrt pattern is POSIX charset, so custom pattern begin at second pattern */
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_no++;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }

  KevFA** nfa_array = (KevFA**)malloc(sizeof (KevFA*) * (nfa_no + 1));
  if (!nfa_array) fatal_error("out of memory", NULL);
  pattern = list->head->next;
  size_t i = 0;
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_array[i++] = nfa_info->fa;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }
  nfa_array[i] = NULL;  /* nfa_array must terminate with NULL */

  KevFA* dfa = kev_nfa_to_dfa(nfa_array, p_acc_mapping);
  free(nfa_array);
  if (!dfa)
    fatal_error("failed to construct dfa", NULL);
  *p_min_dfa = kev_dfa_minimization(dfa, *p_acc_mapping);
  kev_fa_delete(dfa);
  if (!*p_min_dfa)
    fatal_error("failed to minimize dfa", NULL);
  return nfa_no;
}

static void kev_lexgen_control_regularize_mapping(KevFA* dfa, size_t nfa_no, KevPatternList* list, size_t* acc_mapping, size_t** p_pattern_mapping) {
  /* regularize pattern_mapping */
  size_t* nfa_to_pattern = (size_t*)malloc(sizeof (size_t) * nfa_no);
  if (!nfa_to_pattern) fatal_error("out of memory", NULL);
  KevPattern* pattern = list->head->next;
  size_t i = 0;
  size_t pattern_id = 0;
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      nfa_to_pattern[i] = pattern_id;
      nfa_info = nfa_info->next;
    }
    pattern_id++;
    pattern = pattern->next;
  }
  size_t acc_state_number = kev_dfa_accept_state_number(dfa);
  size_t* pattern_mapping = (size_t*)malloc(sizeof (size_t) * acc_state_number);
  for (size_t i = 0; i < acc_state_number; ++i)
    pattern_mapping[i] = nfa_to_pattern[(acc_mapping)[i]];
  *p_pattern_mapping = pattern_mapping;
  free(nfa_to_pattern);
}

static char** kev_lexgen_construct_callback_array(KevFA* dfa, KevPatternList* list, size_t* acc_mapping, size_t nfa_no, size_t* p_arrlen) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  size_t i = 0;
  KevPattern* pattern = list->head->next;
  char** func_names = (char**)malloc(sizeof (char*) * nfa_no);
  if (!func_names) fatal_error("out of meory", NULL);
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      func_names[i++] = nfa_info->name;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }
  char** callbacks = (char**)malloc(sizeof (char*) * state_number);
  if (!callbacks) fatal_error("out of meory", NULL);
  for (size_t i = 0; i < non_acc_number; ++i) {
    callbacks[i] = NULL;
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    char* funcname = func_names[acc_mapping[i - non_acc_number]];
    callbacks[i] = funcname ? funcname : "NULL";
  }
  free(func_names);
  *p_arrlen = state_number;
  return callbacks;
}
