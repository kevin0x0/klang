#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "lexgen/include/lexgen/output.h"
#include "lexgen/include/lexgen/template.h"
#include "lexgen/include/lexgen/error.h"
#include "utils/include/os_spec/dir.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>

static void kev_lexgen_output_table_rust(FILE* output, KevPatternBinary* binary_info);
static void kev_lexgen_output_pattern_mapping_rust(FILE* output, KevPatternBinary* binary_info);
static void kev_lexgen_output_start_rust(FILE* output, KevPatternBinary* binary_info);
static char* kev_lexgen_output_callback_rust(KevPatternBinary* binary_info);
static char* kev_lexgen_output_info_rust(KevPatternBinary* binary_info);
static char* kev_lexgen_output_macro_rust(KevPatternBinary* binary_info);

static void kev_lexgen_output_table_c_cpp(FILE* output, KevPatternBinary* binary_info);
static void kev_lexgen_output_pattern_mapping_c_cpp(FILE* output, KevPatternBinary* binary_info);
static void kev_lexgen_output_start_c_cpp(FILE* output, KevPatternBinary* binary_info);
static char* kev_lexgen_output_callback_c_cpp(KevPatternBinary* binary_info);
static char* kev_lexgen_output_info_c_cpp(KevPatternBinary* binary_info);
static char* kev_lexgen_output_macro_c_cpp(KevPatternBinary* binary_info);

/* convert 'str' to escape string to 'output' buffer,
 * return pointer of next position in buffer
 */
static char* kev_lexgen_output_escape_string(char* output, char* str);

void kev_lexgen_output_set_func(KevOutputFunc* func_group, char* language) {
  if (strcmp(language, "rust") == 0) {
    func_group->output_table = kev_lexgen_output_table_rust;
    func_group->output_pattern_mapping = kev_lexgen_output_pattern_mapping_rust;
    func_group->output_start = kev_lexgen_output_start_rust;
    func_group->output_callback = kev_lexgen_output_callback_rust;
    func_group->output_info = kev_lexgen_output_info_rust;
    func_group->output_macro = kev_lexgen_output_macro_rust;
  } else if (strcmp(language, "c") == 0 ||
           strcmp(language, "cpp") == 0) {
    func_group->output_table = kev_lexgen_output_table_c_cpp;
    func_group->output_pattern_mapping = kev_lexgen_output_pattern_mapping_c_cpp;
    func_group->output_start = kev_lexgen_output_start_c_cpp;
    func_group->output_callback = kev_lexgen_output_callback_c_cpp;
    func_group->output_info = kev_lexgen_output_info_c_cpp;
    func_group->output_macro = kev_lexgen_output_macro_c_cpp;
  } else {
    kev_throw_error("output:", "unsupported language: ", language);
  }
}

static char* kev_lexgen_output_escape_string(char* output, char* str) {
  if (!str) sprintf(output, "NULL");
  char* bufpos = output;
  while (*str != '\0')
    *bufpos++ = *str++;
  return bufpos;
}

void kev_lexgen_output_help(void) {
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    kev_throw_error("output:", "can not get resources directory", NULL);
  char* help_dir = kev_str_concat(resources_dir, "doc/help.txt");
  if (!help_dir)
    kev_throw_error("output:", "out of memory", NULL);
  FILE* help = fopen(help_dir, "r");
  char line[90];
  while (fgets(line, 90, help))
    fputs(line, stdout);
  fclose(help);
  free(help_dir);
  free(resources_dir);
}

void kev_lexgen_output_src(FILE* output, KevOptions* options, KevStringMap* env_var) {
  if (options->strs[KEV_LEXGEN_OUT_SRC_PATH]) {
    char* src_tmpl_path = options->strs[KEV_LEXGEN_SRC_TMPL_PATH];
    FILE* tmpl = fopen(src_tmpl_path, "r");
    if (!tmpl)
      kev_throw_error("output:", "can not open file: ", src_tmpl_path);
    kev_template_convert(output, tmpl, env_var);
    fclose(tmpl);
  }
  /* some languages like C/C++ need a header */
  if (options->strs[KEV_LEXGEN_OUT_INC_PATH]) {
    char* inc_tmpl_path = options->strs[KEV_LEXGEN_INC_TMPL_PATH];
    FILE* tmpl = fopen(inc_tmpl_path, "r");
    FILE* inc_output = fopen(options->strs[KEV_LEXGEN_OUT_INC_PATH], "w");
    if (!tmpl)
      kev_throw_error("output:", "can not open file: ", inc_tmpl_path);
    if (!inc_output)
      kev_throw_error("output:", "can not open file: ", options->strs[KEV_LEXGEN_OUT_INC_PATH]);
    kev_template_convert(inc_output, tmpl, env_var);
    fclose(tmpl);
    fclose(inc_output);
  }
}

static char* kev_lexgen_output_macro_rust(KevPatternBinary* binary_info) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
  return NULL;
}

static char* kev_lexgen_output_info_rust(KevPatternBinary* binary_info) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
  return NULL;
}

static char* kev_lexgen_output_callback_rust(KevPatternBinary* binary_info) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
  return NULL;
}

static void kev_lexgen_output_table_rust(FILE* output, KevPatternBinary* binary_info) {
  size_t state_length = binary_info->state_length;
  size_t charset_size = binary_info->charset_size;
  size_t state_no = binary_info->dfa_state_no;
  char* type = state_length == 8 ? "uint8_t" : "uint16_t";
  void* table =binary_info->table;
  /* output transition table */
  fprintf(output, "static TRANSITION : [[%s;%d];%d] = [\n", type, (int)charset_size, (int)state_no);
  if (charset_size == 128 && state_length == 8) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  [", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[128])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (charset_size == 128 && state_length == 16) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  [", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[128])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (charset_size == 256 && state_length == 8) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  [", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[256])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  } else if (charset_size == 256 && state_length == 16) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  [", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[256])table)[i][j]);
      }
      fputs("\n  ],\n", output);
    }
  }
  fputs("];\n\n", output);
  /* interface function */
  fprintf(output, "fn kev_lexgen_get_transition_table(void) -> &[[%s;%d]] {\n", type, (int)charset_size);
  fprintf(output, "  return TRANSITION;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_pattern_mapping_rust(FILE* output, KevPatternBinary* binary_info) {
  size_t state_no = binary_info->dfa_state_no;
  int* pattern_mapping = binary_info->pattern_mapping;
  /* output accepting state mapping array */
  fprintf(output, "static PATTERN_MAPPING : [i32;%d] = [", (int)state_no);
  for(size_t i = 0; i < state_no; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fprintf(output, "%4d,", (int)pattern_mapping[i]);
  }
  fputs("\n];\n\n", output);
  /* interface function */
  fprintf(output, "fn kev_lexgen_get_pattern_mapping(void) -> &[i32] {\n");
  fprintf(output, "  return PATTERN_MAPPING;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_start_rust(FILE* output, KevPatternBinary* binary_info) {
  /* start state */
  fprintf(output, "static START : usize = %d;\n\n", (int)binary_info->dfa_start);
  /* interface function */
  fprintf(output, "fn kev_lexgen_get_start_state(void) -> usize {\n");
  fprintf(output, "  return START;\n");
  fprintf(output, "}\n\n");
}


static void kev_lexgen_output_table_c_cpp(FILE* output, KevPatternBinary* binary_info) {
  size_t state_length = binary_info->state_length;
  size_t charset_size = binary_info->charset_size;
  size_t state_no = binary_info->dfa_state_no;
  char* type = state_length == 8 ? "uint8_t" : "uint16_t";
  void* table =binary_info->table;
  fputs( "#include <stdint.h>\n", output);
  fputs( "#include <stddef.h>\n", output);
  /* output transition table */
  fprintf(output, "static %s transition[%d][%d] = {\n", type, (int)state_no, (int)charset_size);
  if (charset_size == 128 && state_length == 8) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  {", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[128])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (charset_size == 128 && state_length == 16) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  {", output);
      for (size_t j = 0; j < 128; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[128])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (charset_size == 256 && state_length == 8) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  {", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint8_t (*)[256])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  } else if (charset_size == 256 && state_length == 16) {
    for (size_t i = 0; i < state_no; ++i) {
      fprintf(output, "  /* %d */\n", (int)i);
      fputs("  {", output);
      for (size_t j = 0; j < 256; ++j) {
        if (j % 16 == 0) fputs("\n    ", output);
        fprintf(output, "%4d,", (int)((uint16_t (*)[256])table)[i][j]);
      }
      fputs("\n  },\n", output);
    }
  }
  fputs("};\n\n", output);
  /* interface function */
  fprintf(output, "%s (*kev_lexgen_get_transition_table(void))[%d] {\n", type, (int)charset_size);
  fprintf(output, "  return transition;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_pattern_mapping_c_cpp(FILE* output, KevPatternBinary* binary_info) {
  size_t state_no = binary_info->dfa_state_no;
  int* pattern_mapping = binary_info->pattern_mapping;
  /* output pattern mapping array */
  fprintf(output, "static int pattern_mapping[%d] = {", (int)state_no);
  for(size_t i = 0; i < state_no; ++i) {
    if (i % 16 == 0) fputs("\n  ", output);
    fprintf(output, "%4d,", (int)pattern_mapping[i]);
  }
  fputs("\n};\n\n", output);
  /* interface function */
  fprintf(output, "int* kev_lexgen_get_pattern_mapping(void) {\n");
  fprintf(output, "  return pattern_mapping;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_start_c_cpp(FILE* output, KevPatternBinary* binary_info) {
  /* start state */
  fprintf(output, "static size_t start = %d;\n\n", (int)binary_info->dfa_start);
  /* interface function */
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n\n");
}

static char* kev_lexgen_output_callback_c_cpp(KevPatternBinary* binary_info) {
  char** callbacks = binary_info->callbacks;
  size_t buflen = 256;
  for (size_t i = 0; i < binary_info->dfa_state_no; ++i) {
    if (callbacks[i])
      buflen += 2 * strlen(callbacks[i]) + 15;
    else
      buflen += 4 + 4;
  }

  char* output = (char*)malloc(sizeof (char) * buflen);
  if (!output)
    kev_throw_error("output:", "out of memory", NULL);
  char* bufpos = output;
  for (size_t i = 0; i < binary_info->dfa_state_no; ++i) {
    if (callbacks[i])
      bufpos += sprintf(bufpos, "Callback %s;\n", callbacks[i]);
  }

  bufpos += sprintf(bufpos, "static Callback* callbacks[%d] = {", (int)binary_info->dfa_state_no);
  for(size_t i = 0; i < binary_info->dfa_state_no; ++i) {
    bufpos += sprintf(bufpos, "\n  %s,", callbacks[i] ? callbacks[i] : "NULL");
  }
  bufpos += sprintf(bufpos, "\n};\n\n");
  bufpos += sprintf(bufpos, "Callback** kev_lexgen_get_callbacks(void) {\n");
  bufpos += sprintf(bufpos, "  return callbacks;\n");
  bufpos += sprintf(bufpos, "}\n\n");
  return output;
}

static char* kev_lexgen_output_info_c_cpp(KevPatternBinary* binary_info) {
  size_t buflen = 256;
  char** infos = binary_info->infos;
  for (size_t i = 0; i < binary_info->pattern_no; ++i) {
    buflen += 2 * strlen(infos[i]) + 6;
  }
  char* output = (char*)malloc(sizeof (char) * buflen);
  char* bufpos = output;
  if (!output)
    kev_throw_error("output:", "out of memory", NULL);
  bufpos += sprintf(bufpos, "static const char* info[%d] = {\n", (int)binary_info->pattern_no);
  for (size_t i = 0; i < binary_info->pattern_no; ++i) {
    bufpos += sprintf(bufpos, "  \"");
    bufpos = kev_lexgen_output_escape_string(bufpos, infos[i]);
    bufpos += sprintf(bufpos, "\",\n");
  }
  bufpos += sprintf(bufpos, "};\n\n");
  bufpos += sprintf(bufpos, "const char** kev_lexgen_get_info(void) {\n");
  bufpos += sprintf(bufpos, "  return info;\n");
  bufpos += sprintf(bufpos, "}\n\n");
  return output;
}
static char* kev_lexgen_output_macro_c_cpp(KevPatternBinary* binary_info) {
  size_t buflen = 1;
  KevAddrArray** macros = binary_info->macros;
  int* macro_ids = binary_info->macro_ids;
  for (size_t i = 0; i < binary_info->pattern_no; ++i) {
    size_t arrlen = kev_addrarray_size(macros[i]);
    for (size_t j = 0; j < arrlen; ++j) {
      char* macro_name = kev_addrarray_visit(macros[i], j);
      buflen += strlen(macro_name) + 22 + sizeof (macro_ids[0]) * 8 / 2; /* log(10, 2) < 0.5, * 0.5 -> / 2 */
    }
  }
  char* output = (char*)malloc(sizeof (char) * buflen);
  char* bufpos = output;
  if (!output)
    kev_throw_error("output:", "out of memory", NULL);
  *bufpos = '\0';
  for (size_t i = 0; i < binary_info->pattern_no; ++i) {
    size_t arrlen = kev_addrarray_size(macros[i]);
    for (size_t j = 0; j < arrlen; ++j) {
      char* macro_name = kev_addrarray_visit(macros[i], j);
      bufpos += sprintf(bufpos, "#define ");
      bufpos += sprintf(bufpos, "%s (%d)\n", macro_name, (int)macro_ids[i]);
    }
  }
  return output;
}

