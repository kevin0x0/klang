#include "lexgen/include/lexgen/output.h"
#include "lexgen/include/lexgen/error.h"
#include "lexgen/include/lexgen/dir.h"
#include "template/include/template.h"
#include "utils/include/string/kev_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void kev_lexgen_output_table_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_pattern_mapping_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_start_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_callback_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_info_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_macro_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);

static void kev_lexgen_output_table_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_pattern_mapping_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_start_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_callback_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_info_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);
static void kev_lexgen_output_macro_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var);

static void kev_lexgen_output_escape_str(FILE* output, const char* str);

void kev_lexgen_output_set_func(KevLOutputFuncGroup* func_group, const char* language) {
  if (strcmp(language, "rust") == 0) {
    func_group->output_table = (KevLOutputFunc*)kev_lexgen_output_table_rust;
    func_group->output_pattern_mapping = (KevLOutputFunc*)kev_lexgen_output_pattern_mapping_rust;
    func_group->output_start = (KevLOutputFunc*)kev_lexgen_output_start_rust;
    func_group->output_callback = (KevLOutputFunc*)kev_lexgen_output_callback_rust;
    func_group->output_info = (KevLOutputFunc*)kev_lexgen_output_info_rust;
    func_group->output_macro = (KevLOutputFunc*)kev_lexgen_output_macro_rust;
  } else if (strcmp(language, "c") == 0 ||
             strcmp(language, "cpp") == 0) {
    func_group->output_table = (KevLOutputFunc*)kev_lexgen_output_table_c_cpp;
    func_group->output_pattern_mapping = (KevLOutputFunc*)kev_lexgen_output_pattern_mapping_c_cpp;
    func_group->output_start = (KevLOutputFunc*)kev_lexgen_output_start_c_cpp;
    func_group->output_callback = (KevLOutputFunc*)kev_lexgen_output_callback_c_cpp;
    func_group->output_info = (KevLOutputFunc*)kev_lexgen_output_info_c_cpp;
    func_group->output_macro = (KevLOutputFunc*)kev_lexgen_output_macro_c_cpp;
  } else {
    kev_throw_error("output:", "unsupported language: ", language);
  }
}

void kev_lexgen_output_help(void) {
  char* resources_dir = kev_get_lexgen_resources_dir();
  if (!resources_dir)
    kev_throw_error("output:", "can not get resources directory", NULL);
  char* help_dir = kev_str_concat(resources_dir, "doc/help.txt");
  if (!help_dir)
    kev_throw_error("output:", "out of memory", NULL);
  FILE* help = fopen(help_dir, "r");
  if (!help)
    kev_throw_error("output:", "can not open help file", NULL);
  char line[90];
  while (fgets(line, 90, help))
    fputs(line, stdout);
  fclose(help);
  free(help_dir);
  free(resources_dir);
}

void kev_lexgen_output(const char* output_path, const char* tmpl_path, KevStringMap* env_var, KevFuncMap* funcs) {
  FILE* output = fopen(output_path, "w");
  if (!output)
    kev_throw_error("output:", "can not open file: ", output_path);
  FILE* tmpl = fopen(tmpl_path, "r");
  if (!tmpl)
    kev_throw_error("output:", "can not open file: ", tmpl_path);
  kev_template_convert(output, tmpl, env_var, funcs);
  fclose(tmpl);
  fclose(output);
}

static void kev_lexgen_output_callback_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
}

static void kev_lexgen_output_info_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
}

static void kev_lexgen_output_macro_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  /* TODO: support rust */
  kev_throw_error("output:", "rust is currently not supported", NULL);
}

static void kev_lexgen_output_table_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  size_t state_length = table_info->state_length;
  size_t charset_size = table_info->charset_size;
  size_t state_no = table_info->dfa_state_no;
  const char* type = state_length == 8 ? "uint8_t" : "uint16_t";
  void* table =table_info->table;
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

static void kev_lexgen_output_pattern_mapping_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  size_t state_no = table_info->dfa_state_no;
  int* pattern_mapping = table_info->pattern_mapping;
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

static void kev_lexgen_output_start_rust(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  /* start state */
  fprintf(output, "static START : usize = %d;\n\n", (int)table_info->dfa_start);
  /* interface function */
  fprintf(output, "fn kev_lexgen_get_start_state(void) -> usize {\n");
  fprintf(output, "  return START;\n");
  fprintf(output, "}\n\n");
}


static void kev_lexgen_output_table_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  size_t state_length = table_info->state_length;
  size_t charset_size = table_info->charset_size;
  size_t state_no = table_info->dfa_state_no;
  const char* type = state_length == 8 ? "uint8_t" : "uint16_t";
  void* table =table_info->table;
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

static void kev_lexgen_output_pattern_mapping_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  size_t state_no = table_info->dfa_state_no;
  int* pattern_mapping = table_info->pattern_mapping;
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

static void kev_lexgen_output_start_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  /* start state */
  fprintf(output, "static size_t start = %d;\n\n", (int)table_info->dfa_start);
  /* interface function */
  fprintf(output, "size_t kev_lexgen_get_start_state(void) {\n");
  fprintf(output, "  return start;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_callback_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  char** callbacks = table_info->callbacks;
  for (size_t i = 0; i < table_info->dfa_state_no; ++i) {
    if (callbacks[i])
      fprintf(output, "Callback %s;\n", callbacks[i]);
  }

  fprintf(output, "static Callback* callbacks[%d] = {", (int)table_info->dfa_state_no);
  for(size_t i = 0; i < table_info->dfa_state_no; ++i) {
    fprintf(output, "\n  %s,", callbacks[i] ? callbacks[i] : "NULL");
  }
  fprintf(output, "\n};\n\n");
  fprintf(output, "Callback** kev_lexgen_get_callbacks(void) {\n");
  fprintf(output, "  return callbacks;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_info_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  char** infos = table_info->infos;
  fprintf(output, "static const char* info[%d] = {\n", (int)table_info->pattern_no);
  for (size_t i = 0; i < table_info->pattern_no; ++i) {
    fprintf(output, "  \"");
    if (infos[i]) {
      kev_lexgen_output_escape_str(output, infos[i]);
    } else {
      fprintf(output, "NULL");
    }
    fprintf(output, "\",\n");
  }
  fprintf(output, "};\n\n");
  fprintf(output, "const char** kev_lexgen_get_info(void) {\n");
  fprintf(output, "  return info;\n");
  fprintf(output, "}\n\n");
}

static void kev_lexgen_output_macro_c_cpp(FILE* output, KevLTableInfos* table_info, KevStringMap* env_var) {
  KArray** macros = table_info->macros;
  int* macro_ids = table_info->macro_ids;
  for (size_t i = 0; i < table_info->pattern_no; ++i) {
    size_t arrlen = karray_size(macros[i]);
    for (size_t j = 0; j < arrlen; ++j) {
      const char* macro_name = (const char*)karray_access(macros[i], j);
      fprintf(output, "#define ");
      fprintf(output, "%s (%d)\n", macro_name, (int)macro_ids[i]);
    }
  }
}

static void kev_lexgen_output_escape_str(FILE* output, const char* str) {
  for (const char* ptr = str; *ptr != '\0'; ++ptr) {
    switch (*ptr) {
      case '\n':
        fputs("\\n", output);
        break;
      case '\t':
        fputs("\\t", output);
        break;
      case '\\':
        fputs("\\\\", output);
        break;
      case '\r':
        fputs("\\r", output);
        break;
      case '\"':
        fputs("\\\"", output);
        break;
      case '\'':
        fputs("\\\'", output);
        break;
      default:
        fputc(*ptr, output);
        break;
    }
  }
}
