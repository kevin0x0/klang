#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "lexgen/include/lexgen/output.h"
#include "lexgen/include/lexgen/template.h"
#include "utils/include/dir.h"

#include <stdlib.h>
#include <string.h>

static void kev_lexgen_output_table_rust(FILE* output, KevFA* dfa, size_t* pattern_mapping, void* table, KevOptions* options);
static void kev_lexgen_output_table_c_cpp(FILE* output, KevFA* dfa, size_t* pattern_mapping, void* table, KevOptions* options);
static void kev_lexgen_output_callback_rust(FILE* output, char** callbacks, size_t arrlen);
static void kev_lexgen_output_callback_c_cpp(FILE* output, char** callbacks, size_t arrlen);
static void kev_lexgen_output_info_rust(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options);
static void kev_lexgen_output_info_c_cpp(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options);

static void kev_lexgen_output_escape_string(FILE* output, char* str);
static uint8_t (*kev_lexgen_output_get_trans_256_u8(KevFA* dfa))[256];
static uint8_t (*kev_lexgen_output_get_trans_128_u8(KevFA* dfa))[128];
static uint16_t (*kev_lexgen_output_get_trans_256_u16(KevFA* dfa))[256];
static uint16_t (*kev_lexgen_output_get_trans_128_u16(KevFA* dfa))[128];

static void fatal_error(char* info, char* info2);

void kev_lexgen_output_table(FILE* output, KevFA* dfa, size_t* pattern_mapping, KevOptions* options) {
  void* trans = NULL;
  if (options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII &&
      options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    trans = kev_lexgen_output_get_trans_128_u8(dfa);
  } else if (options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII &&
             options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    trans = kev_lexgen_output_get_trans_128_u16(dfa);
  } else if (options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_UTF8 &&
             options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    trans = kev_lexgen_output_get_trans_256_u8(dfa);
  } else if (options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_UTF8 &&
             options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    trans = kev_lexgen_output_get_trans_256_u16(dfa);
  } else {
    fatal_error("internal error occurred in kev_lexgen_output_table()", NULL);
  }
  if (!trans)
    fatal_error("failed to generate transition table, try --width=16 and --charset=utf-8", NULL);
  if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "rust") == 0)
    kev_lexgen_output_table_rust(output, dfa, pattern_mapping, trans, options);
  else if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "c") == 0 ||
           strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "cpp") == 0)
    kev_lexgen_output_table_c_cpp(output, dfa, pattern_mapping, trans, options);
  else
    fatal_error("unsupported language: ", options->strs[KEV_LEXGEN_LANG_NAME]);
  free(trans);
}

static void kev_lexgen_output_table_rust(FILE* output, KevFA* dfa, size_t* pattern_mapping, void* table, KevOptions* options) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  char* type = options->opts[KEV_LEXGEN_OPT_WIDTH] == 8 ? "u8" : "u16";
  size_t arrlen = options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII ? 128 : 256;
  /* output transition table */
  fprintf(output, "static TRANSITION : [[%s;%d];%d] = [\n", type, (int)arrlen, (int)state_number);
  if (arrlen == 128 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  [", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[128])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (arrlen == 128 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  [", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[128])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (arrlen == 256 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  [", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[256])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (arrlen == 256 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  [", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[256])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  }
  fputs("];\n", output);
  /* output accepting state mapping array */
  fprintf(output, "static PATTERN_MAPPING : [i32;%d] = [", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fputs("  -1,", output);
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fputs("\n];\n", output);
  /* start state */
  fprintf(output, "static START : usize = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "fn kev_lexgen_get_transition_table(void) -> &[[%s;%d]] {\n", type, (int)arrlen);
  fprintf(output, "  return TRANSITION;\n");
  fprintf(output, "}\n");
  fprintf(output, "fn kev_lexgen_get_pattern_mapping(void) -> &[i32] {\n");
  fprintf(output, "  return PATTERN_MAPPING;\n");
  fprintf(output, "}\n");
  fprintf(output, "fn kev_lexgen_get_start_state(void) -> usize {\n");
  fprintf(output, "  return START;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_output_table_c_cpp(FILE* output, KevFA* dfa, size_t* pattern_mapping, void* table, KevOptions* options) {
  size_t non_acc_number = kev_dfa_non_accept_state_number(dfa);
  size_t state_number = kev_dfa_accept_state_number(dfa) + non_acc_number;
  char* type = options->opts[KEV_LEXGEN_OPT_WIDTH] == 8 ? "uint8_t" : "uint16_t";
  size_t arrlen = options->opts[KEV_LEXGEN_OPT_CHARSET] == KEV_LEXGEN_OPT_CHARSET_ASCII ? 128 : 256;
  fputs( "#include <stdint.h>\n", output);
  fputs( "#include <stddef.h>\n", output);
  /* output transition table */
  fprintf(output, "static %s transition[%d][%d] = {\n", type, (int)state_number, (int)arrlen);
  if (arrlen == 128 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  {", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[128])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (arrlen == 128 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  {", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[128])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (arrlen == 256 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 8) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  {", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[256])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (arrlen == 256 && options->opts[KEV_LEXGEN_OPT_WIDTH] == 16) {
    for (size_t i = 0; i < state_number; ++i) {
      fputs("  {", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[256])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  }
  fputs("};\n", output);
  /* output pattern mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_number);
  for (size_t i = 0; i < non_acc_number; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fputs("  -1,", output);
  }
  for(size_t i = non_acc_number; i < state_number; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fprintf(output, "%4d,", (int)pattern_mapping[i - non_acc_number]);
  }
  fputs("\n};\n", output);
  /* start state */
  fprintf(output, "static size_t start = %d;\n", (int)dfa->start_state->id);
  /* interface function */
  fprintf(output, "%s (*kev_lexgen_get_transition_table(void))[%d] {\n", type, (int)arrlen);
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n");
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n");
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n");
}

void kev_lexgen_output_callback(FILE* output, char** callbacks, KevOptions* options, size_t arrlen) {
  if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "rust") == 0)
    kev_lexgen_output_callback_rust(output, callbacks, arrlen);
  else if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "c") == 0 ||
           strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "cpp") == 0)
    kev_lexgen_output_callback_c_cpp(output, callbacks, arrlen);
  else
    fatal_error("unsupported language: ", options->strs[KEV_LEXGEN_LANG_NAME]);
}

void kev_lexgen_output_info(FILE* output, KevPatternList* list, size_t* pattern_mapping, KevOptions* options) {
  if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "rust") == 0)
    kev_lexgen_output_info_rust(output, list, pattern_mapping, options->opts);
  else if (strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "c") == 0 ||
           strcmp(options->strs[KEV_LEXGEN_LANG_NAME], "cpp") == 0)
    kev_lexgen_output_info_c_cpp(output, list, pattern_mapping, options->opts);
  else
    fatal_error("unsupported language: ", options->strs[KEV_LEXGEN_LANG_NAME]);
}

static void kev_lexgen_output_callback_c_cpp(FILE* output, char** callbacks, size_t arrlen) {
  for (size_t i = 0; i < arrlen; ++i) {
    if (callbacks[i])
      fprintf(output, "Callback %s;\n", callbacks[i]);
  }

  fprintf(output, "static Callback* callback[%d] = {", (int)arrlen);
  for(size_t i = 0; i < arrlen; ++i) {
    fprintf(output, "\n  %s,", callbacks[i] ? callbacks[i] : "NULL");
  }
  fprintf(output, "\n};\n");
}

static void kev_lexgen_output_info_c_cpp(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options) {
  size_t pattern_number = 0;
  KevPattern* pattern = list->head->next;
  while (pattern) {
    pattern_number++;
    pattern = pattern->next;
  }
  fprintf(output, "static char* info[%d] = {\n", (int)pattern_number);
  pattern = list->head->next;
  while (pattern) {
    fputs("  \"", output);
    kev_lexgen_output_escape_string(output, pattern->name);
    fputs("\",\n", output);
    pattern = pattern->next;
  }
  fprintf(output, "};\n");
  fprintf(output, "char** kev_lexgen_get_callback_array(void) {\n");
  fprintf(output, "  return info;\n");
  fprintf(output, "}\n");
}

static void kev_lexgen_output_info_rust(FILE* output, KevPatternList* list, size_t* pattern_mapping, int* options) {
  /* TODO: support rust */
  fatal_error("rust is currently not supported", NULL);
}

static void kev_lexgen_output_callback_rust(FILE* output, char** callbacks, size_t arrlen) {
  /* TODO: support rust */
  fatal_error("rust is currently not supported", NULL);
}

static void kev_lexgen_output_escape_string(FILE* output, char* str) {
  if (!str) fprintf(output, "NULL");
  while (*str != '\0') {
    if (*str == '\'' || *str == '\"')
      fprintf(output, "\\%c", *str);
    else
      fputc(*str, output);
    str++;
  }
}

void kev_lexgen_output_help(void) {
  printf("Usage:\n");
  printf("  lexgen [ opt [opt-value]]...\n");
  printf("options:\n");
  printf("  -h --help                        Show this message.\n");
  printf("  -i --in                          Input file path.\n");
  printf("  -o <path> --out <path>           Output file path.\n");
  printf("  --out-inc <path>                 Output include file path. This option works\n");
  printf("                                   only when option -s=s, -s=a or -s=sh is\n");
  printf("                                   specified and the language has header file,\n");
  printf("                                   such as C/C++.\n");
  printf("  -s=<value> --stage=<value>       Specify generation stage.\n");
  printf("    -s value:\n");
  printf("      t[rans[ition]]               Generate the transition table only. This is\n");
  printf("                                   the default value.\n");
  printf("      s[ource]                     Generate the source of lexer.\n");
  printf("      a[rchive]                    Generate the archive of lexer.\n");
  printf("      sh[ared]                     Generate the shared object file of lexer.\n");
  printf("  -l=<value> --lang[uage]=<value>  Specify language.\n");
  printf("    -l value:\n");
  printf("      c                            This is the default value.\n");
  printf("      cpp | cxx | cc | cx | c++\n");
  printf("      rust | rs                    Currently not supported.\n");
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
  printf("                                   DFA can not exceed 255, This is the default\n");
  printf("                                   value.\n");
  printf("      16                           Use uint16_t. This means the state number of\n");
  printf("                                   DFA can not exceed 65535.\n");
  printf("  --charset=<value>                Specify charset, This only affect the\n");
  printf("                                   transtion table.\n");
  printf("    --charset value:\n");
  printf("      utf-8                        Transition table will accept all 256 input\n");
  printf("                                   characters. This is the default value.\n");
  printf("      ascii                        Transition table only accept 0-127.\n");
}

void kev_lexgen_output_src(KevOptions* options, KevStringMap* tmpl_map, char** callbacks, size_t arrlen) {
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
    fatal_error("can not open file(maybe this language is not supported): ", src_path);
  FILE* output = fopen(options->strs[KEV_LEXGEN_OUT_SRC_PATH], "w");
  if (!output)
    fatal_error("can not open file: ", options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
  kev_template_convert(output, tmpl, tmpl_map);
  kev_lexgen_output_callback(output, callbacks, options, arrlen);
  fclose(output);
  fclose(tmpl);
  /* some languages like C/C++ need header */
  if (options->strs[KEV_LEXGEN_OUT_INC_PATH]) {
    strcpy(src_path, resources_dir);
    strcat(src_path, "lexer/");
    strcat(src_path, options->strs[KEV_LEXGEN_LANG_NAME]);
    strcat(src_path, "/inc.tmpl");
    tmpl = fopen(src_path, "r");
    if (!tmpl)
      fatal_error("can not open file: ", src_path);
    output = fopen(options->strs[KEV_LEXGEN_OUT_INC_PATH], "w");
    if (!output)
      fatal_error("can not open file: ", options->strs[KEV_LEXGEN_OUT_SRC_PATH]);
    kev_template_convert(output, tmpl, tmpl_map);
    fclose(output);
    fclose(tmpl);
  }
  free(src_path);
  free(resources_dir);
}

void kev_lexgen_output_arc(char* compiler, KevOptions* options) {
  /* TODO: output archive file */
  fatal_error("generating archive file currently is not supported", NULL);
}

void kev_lexgen_output_sha(char* compiler, KevOptions* options) {
  /* TODO: output shared object file */
  fatal_error("generating shared object file currently is not supported", NULL);
}

static void fatal_error(char* info, char* info2) {
  fputs("fatal: ", stderr);
  if (info)
    fputs(info, stderr);
  if (info2)
    fputs(info2, stderr);
  fputs("\nterminated\n", stderr);
  exit(EXIT_FAILURE);
}

uint8_t (*kev_lexgen_output_get_trans_256_u8(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[256] = (uint8_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint8_t (*kev_lexgen_output_get_trans_128_u8(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 256) return NULL;
  uint8_t (*table)[128] = (uint8_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint8_t)edge->attr] = (uint8_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_lexgen_output_get_trans_256_u16(KevFA* dfa))[256] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[256] = (uint16_t(*)[256])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 256; ++j) {
      table[i][j] = 255;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 255)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}

uint16_t (*kev_lexgen_output_get_trans_128_u16(KevFA* dfa))[128] {
  size_t state_number = kev_fa_state_assign_id(dfa, 0);
  if (state_number >= 65536) return NULL;
  uint16_t (*table)[128] = (uint16_t(*)[128])malloc(sizeof (*table) * state_number);
  if (!table) return NULL;
  for (size_t i = 0; i < state_number; ++i) {
    for (size_t j = 0; j < 128; ++j) {
      table[i][j] = 127;
    }
  }
  KevGraphNode* node = kev_fa_get_states(dfa);
  while (node) {
    KevGraphEdge* edge = kev_graphnode_get_edges(node);
    uint64_t id = node->id;
    while (edge) {
      if ((uint64_t)edge->attr <= 127)
        table[id][(uint16_t)edge->attr] = (uint16_t)edge->node->id;
      edge = edge->next;
    }
    node = node->next;
  }
  return table;
}
