#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/parser/parser.h"
#include "lexgen/include/parser/hashmap/strfa_map.h"
#include "lexgen/include/parser/list/pattern_list.h"
#include "lexgen/include/tablegen/trans_table.h"
#include "utils/include/dir.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void fatal_error(char* info, char* info2);

static int kev_lexgen_parse(FILE* input, KevParserState* parser_state);
/* return number of NFAs */
static size_t kev_lexgen_generate(KevPatternList* list, KevStringFaMap* map, KevFA** p_min_dfa, size_t** p_acc_mapping);
/* output transition table, acc_mapping array and start_state. */
static void kev_lexgen_output_dfa(FILE* output, KevFA* dfa, uint64_t* acc_mapping, int* options);
static void kev_lexgen_output_callback(FILE* output, KevFA* dfa, KevPatternList* list, size_t* acc_mapping, int* options);
static void kev_lexgen_output_info(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options);

static void kev_lexgen_regularize_mapping(KevFA* dfa, size_t nfa_no, KevPatternList* list, size_t* acc_mapping, size_t** p_pattern_mapping);

static void kev_lexgen_print_table_128_u8(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint8_t (*table)[128]);
static void kev_lexgen_print_table_128_u16(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint16_t (*table)[128]);
static void kev_lexgen_print_table_256_u8(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint8_t (*table)[256]);
static void kev_lexgen_print_table_256_u16(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint16_t (*table)[256]);

static void kev_lexgen_gen_src(char* language, KevOptions* options);

static void kev_output_escape_string(FILE* output, char* str);
static void kev_lexgen_help(void);

void kev_lexgen_control(KevOptions* options) {
  if (options->opts[KEV_LEXGEN_OPT_HELP]) {
    kev_lexgen_help();
    return;
  }
  FILE* input = fopen(options->strs[KEV_LEXGEN_INPUT_PATH], "r");
  if (!input) {
    fatal_error("can not open file:", options->strs[KEV_LEXGEN_INPUT_PATH]);
  }
  FILE* output = fopen(options->strs[KEV_LEXGEN_OUT_TAB_PATH], "w");
  if (!output) {
    fclose(input);
    fatal_error("can not open file: ", options->strs[KEV_LEXGEN_OUT_TAB_PATH]);
  }
  KevParserState parser_state;
  if (!kev_lexgenparser_init(&parser_state)) {
    fclose(input);
    fclose(output);
    fatal_error("failed to initialize parser", NULL);
  }
  int error_number = kev_lexgen_parse(input, &parser_state);
  if (error_number != 0) {
    fprintf(stderr, "%d error(s) detected.\n", error_number);
    exit(EXIT_SUCCESS);
  }
  KevFA* dfa;
  size_t* acc_mapping;
  size_t nfa_no = kev_lexgen_generate(&parser_state.list, &parser_state.nfa_map, &dfa, &acc_mapping);
  kev_lexgen_output_dfa(output, dfa, acc_mapping, options->opts);
  if (options->opts[KEV_LEXGEN_OPT_PUT_CALLBACK])
    kev_lexgen_output_callback(output, dfa, &parser_state.list, acc_mapping, options->opts);
  size_t* pattern_mapping = NULL;
  kev_lexgen_regularize_mapping(dfa, nfa_no, &parser_state.list, acc_mapping, &pattern_mapping);
  if (options->opts[KEV_LEXGEN_OPT_PUT_INFO]) {
    kev_lexgen_output_info(output, &parser_state.list, pattern_mapping, options->opts);
  }
  if (options->opts[KEV_LEXGEN_OPT_STAGE] >= KEV_LEXGEN_OPT_STA_SRC) {
    options->strs[KEV_LEXGEN_LANG_NAME];
  }
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

static int kev_lexgen_parse(FILE* input, KevParserState* parser_state) {
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

static size_t kev_lexgen_generate(KevPatternList* list, KevStringFaMap* map, KevFA** p_min_dfa, size_t** p_acc_mapping) {
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

static void kev_lexgen_regularize_mapping(KevFA* dfa, size_t nfa_no, KevPatternList* list, size_t* acc_mapping, size_t** p_pattern_mapping) {
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

static void kev_lexgen_output_dfa(FILE* output, KevFA* dfa, size_t* pattern_mapping, int* options) {
  if (options[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII &&
      options[KEV_LEXGEN_OPT_WIDTH] == 8) {
    uint8_t (*trans)[128] = kev_get_trans_128_u8(dfa);
    if (!trans)
      fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8", NULL);
    kev_lexgen_print_table_128_u8(output, dfa, pattern_mapping, trans);
    free(trans);
  } else if (options[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII &&
             options[KEV_LEXGEN_OPT_WIDTH] == 16) {
    uint16_t (*trans)[128] = kev_get_trans_128_u16(dfa);
    if (!trans)
      fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8");
    kev_lexgen_print_table_128_u16(output, dfa, pattern_mapping, trans);
    free(trans);
  } else if (options[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_UTF8 &&
             options[KEV_LEXGEN_OPT_WIDTH] == 8) {
    uint8_t (*trans)[256] = kev_get_trans_256_u8(dfa);
    if (!trans)
      fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8");
    kev_lexgen_print_table_256_u8(output, dfa, pattern_mapping, trans);
    free(trans);
  } else if (options[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_UTF8 &&
             options[KEV_LEXGEN_OPT_WIDTH] == 16) {
    uint16_t (*trans)[256] = kev_get_trans_256_u16(dfa);
    if (!trans)
      fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8");
    kev_lexgen_print_table_256_u16(output, dfa, pattern_mapping, trans);
    free(trans);
  }
}

static void kev_lexgen_print_table_128_u8(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint8_t (*table)[128]) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  /* output transition table */
  fprintf(output, "static uint8_t transition[%d][128] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(output, "  {");
    for (size_t j = 0; j < 128; ++j) {
      if (j % 16 == 0) fprintf(output, "\n    ");
      fprintf(output, "%4d,", (int)table[i][j]);
    }
    fprintf(output, "\n  },\n");
  }
  fprintf(output, "};\n");
  /* output accepting state mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "  -1,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fprintf(output, "\n};\n");
  /* start state */
  fprintf(output, "static size_t start = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "uint8_t (*kev_lexgen_get_transition_table(void))[128] {\n");
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n");
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n");
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_print_table_128_u16(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint16_t (*table)[128]) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  /* output transition table */
  fprintf(output, "static uint16_t transition[%d][128] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(output, "  {");
    for (size_t j = 0; j < 128; ++j) {
      if (j % 16 == 0) fprintf(output, "\n    ");
      fprintf(output, "%4d,", (int)table[i][j]);
    }
    fprintf(output, "\n  },\n");
  }
  fprintf(output, "};\n");
  /* output accepting state mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "  -1,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fprintf(output, "\n};\n");
  /* start state */
  fprintf(output, "static size_t start = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "uint16_t (*kev_lexgen_get_transition_table(void))[128] {\n");
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n");
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n");
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_print_table_256_u8(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint8_t (*table)[256]) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  /* output transition table */
  fprintf(output, "static uint8_t transition[%d][256] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(output, "  {");
    for (size_t j = 0; j < 256; ++j) {
      if (j % 16 == 0) fprintf(output, "\n    ");
      fprintf(output, "%4d,", (int)table[i][j]);
    }
    fprintf(output, "\n  },\n");
  }
  fprintf(output, "};\n");
  /* output accepting state mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "  -1,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fprintf(output, "\n};\n");
  /* start state */
  fprintf(output, "static size_t start = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "uint8_t (*kev_lexgen_get_transition_table(void))[256] {\n");
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n");
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n");
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_print_table_256_u16(FILE* output, KevFA* dfa, size_t* pattern_mapping, uint16_t (*table)[256]) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  /* output transition table */
  fprintf(output, "static uint16_t transition[%d][256] = {\n", (int)state_number);
  for (size_t i = 0; i < state_number; ++i) {
    fprintf(output, "  {");
    for (size_t j = 0; j < 256; ++j) {
      if (j % 16 == 0) fprintf(output, "\n    ");
      fprintf(output, "%4d,", (int)table[i][j]);
    }
    fprintf(output, "\n  },\n");
  }
  fprintf(output, "};\n");
  /* output accepting state mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "  -1,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fprintf(output, "\n  ");
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fprintf(output, "\n};\n");
  /* start state */
  fprintf(output, "static size_t start = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "uint16_t (*kev_lexgen_get_transition_table(void))[256] {\n");
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n");
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n");
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_output_callback(FILE* output, KevFA* dfa, KevPatternList* list, size_t* acc_mapping, int* options) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  size_t i = 0;
  KevPattern* pattern = list->head->next;
  char** func_names = (char**)malloc(sizeof (char*) * state_number);
  if (!func_names) fatal_error("out of meory", NULL);
  while (pattern) {
    KevFAInfo* nfa_info = pattern->fa_info;
    while (nfa_info) {
      if (nfa_info->name)
        fprintf(output, "Callback %s;\n", nfa_info->name);
      func_names[i] = nfa_info->name;
      nfa_info = nfa_info->next;
    }
    pattern = pattern->next;
  }
  fprintf(output, "typedef void (*Callback)(Token*, uint8_t*, uint8_t*);\n");
  fprintf(output, "static Callback pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    fprintf(output, "\n  NULL,");
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    fprintf(output, "\n  %s,", func_names[acc_mapping[i]] ? func_names[acc_mapping[i]] : "NULL");
  }
  fprintf(output, "\n};\n");
  free(func_names);
  fprintf(output, "Callback kev_lexgen_get_callback_array(void) {\n");
  fprintf(output, "  return callback;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_output_info(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options) {
  /* TODO: output information about token's name */
  size_t pattern_number = 0;
  KevPattern* pattern = list->head->next;
  while (pattern) {
    pattern_number++;
    pattern = pattern->next;
  }
  fprintf(output, "static char* info[%d] = {\n", (int)pattern_number);
  pattern = list->head->next;
  while (pattern) {
    fputc(' ', output);
    fputc(' ', output);
    kev_output_escape_string(output, pattern->name);
    fputc(',', output);
    fputc('\n', output);
    pattern = pattern->next;
  }
  fprintf(output, "};\n");
  fprintf(output, "char** kev_lexgen_get_callback_array(void) {\n");
  fprintf(output, "  return info;\n");
  fprintf(output, "}\n");
}

static void kev_output_escape_string(FILE* output, char* str) {
  if (!str) fprintf(output, "NULL");
  while (*str != '\0') {
    if (*str == '\'' || *str == '\"')
      fprintf(output, "\\%c", *str);
    else
      fputc(*str, output);
    str++;
  }
}

static void kev_lexgen_help(void) {
  printf("Usage:\n");
  printf("  lexgen [ opt [opt-value]]...\n");
  printf("options:\n");
  printf("  -h --help                        Show this message.\n");
  printf("  -i --in[put]                     Input file path.\n");
  printf("  -o <path> --out[put] <path>      Output file path.\n");
  printf("  --out-src <path>                 Output source file path.\n");
  printf("  --out-inc <path>                 Output include file path.\n");
  printf("  -s=<value> --stage=<value>       Specify generation stage.\n");
  printf("    -p value:\n");
  printf("      trans[ition]                 Generate the transition table only. This is\n");
  printf("                                   the default value.\n");
  printf("      s[ource]                     Generate the source of lexer.\n");
  printf("      a[rchive]                    Generate the archive of lexer.\n");
  printf("      sh[ared]                     Generate the shared object file of lexer.\n");
  printf("  --out[put]-info                  Output token name array.\n");
  printf("  --out[put]-callback              Output callback function array.\n");
  printf("  -l=<value> --lang[uage]=<value>  Specify language. Ignore the case of <value>.\n");
  printf("    -l value:\n");
  printf("      c\n");
  printf("      cpp | cxx | cc | cx | c++\n");
  printf("      ru[st]                       Currently not supported.\n");
  printf("  --build-tool=<value>             Specify compiler when generate archive or\n");
  printf("                                   shared object file.\n");
  printf("    --build-tool value:\n");
  printf("      gcc\n");
  printf("      g++\n");
  printf("      clang\n");
  printf("      clang++\n");
  printf("      cl\n");
  printf("  --length=<value>                 Specify integer length that transition table\n");
  printf("                                   uses.");
  printf("    --length value:\n");
  printf("      8                            Use uint8_t. This means the state number of\n");
  printf("                                   DFA can not exceed 255.\n");
  printf("      16                           Use uint16_t. This means the state number of\n");
  printf("                                   DFA can not exceed 65535.\n");
  printf("  --charset=<value>                Specify charset, This only affect the\n");
  printf("                                   transtion table.\n");
  printf("    --charset value:\n");
  printf("      utf-8                        Transition table will accept all 256 input\n");
  printf("                                   characters.\n");
  printf("      ascii                        Transition table only accept 0-127.\n");
}

static void kev_lexgen_gen_src(char* language, KevOptions* options) {
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    fatal_error("can not get resources directory", NULL);
  char* src_path = (char*)malloc(sizeof (char) * (strlen(resources_dir) + strlen("lexer/") + strlen("lexer.") + 32 + 1));
  if (!src_path)
    fatal_error("out of memory", NULL);
  strcpy(src_path, resources_dir);
  strcat(src_path, "lexer/");
  strcat(src_path, options->strs[KEV_LEXGEN_LANG_NAME]);
  strcat(src_path, "/src.tmpl");
  FILE* tmpl = fopen(src_path, "r");
  if (!tmpl)
  FILE* output = fopen(options->strs[KEV_LEXGEN_OUT_SRC_PATH], "w");
}
