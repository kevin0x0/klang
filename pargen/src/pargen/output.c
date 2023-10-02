#include "pargen/include/pargen/output.h"
#include "pargen/include/pargen/convert.h"
#include "pargen/include/pargen/error.h"
#include "pargen/include/pargen/dir.h"
#include "template/include/template.h"
#include "utils/include/string/kev_string.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define KEV_PARGEN_ATTR_IDX_FMT_PLACEHOLDER   ((char)255)


static void kev_pargen_output_symbol_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_start_state_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_action_table_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_goto_table_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_rule_info_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_callback_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);
static void kev_pargen_output_state_symbol_mapping_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var);


static void kev_pargen_output_escape_str(FILE* out, const char* str);
static void kev_pargen_output_simple_replace(FILE* out, const char* begin, const char* end, char placeholder, const char* fmt);


void kev_pargen_output_help(void) {
  char* resources_dir = kev_get_pargen_resources_dir();
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

void kev_pargen_output_lrinfo(const char* collecinfo_path, const char* actioninfo_path,
                              const char* gotoinfo_path, const char* symbolinfo_path,
                              KevLRCollection* collec, KevLRTable* table) {
  if (collecinfo_path) {
    FILE* out = fopen(collecinfo_path, "w");
    if (!out)
      kev_throw_error("output:", "can not open file: ", collecinfo_path);
    if (!kev_lr_print_collection(out, collec, true))
      kev_throw_error("output:", "out of memory", ", failed to print collection");
    fclose(out);
  }
  if (symbolinfo_path) {
    FILE* out = fopen(symbolinfo_path, "w");
    if (!out)
      kev_throw_error("output:", "can not open file: ", symbolinfo_path);
    if (!kev_lr_print_symbols(out, collec))
      kev_throw_error("output:", "out of memory", ", failed to print symbol information");
    fclose(out);
  }
  if (actioninfo_path) {
    FILE* out = fopen(actioninfo_path, "w");
    if (!out)
      kev_throw_error("output:", "can not open file: ", actioninfo_path);
    kev_lr_print_action_table(out, table);
    fclose(out);
  }
  if (gotoinfo_path) {
    FILE* out = fopen(gotoinfo_path, "w");
    if (!out)
      kev_throw_error("output:", "can not open file: ", gotoinfo_path);
    kev_lr_print_goto_table(out, table);
    fclose(out);
  }

}

void kev_pargen_output_set_func(KevPOutputFuncGroup* func_group, const char* language) {
  if (strcmp(language, "c") == 0 || strcmp(language, "cpp") == 0) {
    func_group->output_action_table = (KevPOutputFunc*)kev_pargen_output_action_table_c_cpp;
    func_group->output_goto_table = (KevPOutputFunc*)kev_pargen_output_goto_table_c_cpp;
    func_group->output_start_state = (KevPOutputFunc*)kev_pargen_output_start_state_c_cpp;
    func_group->output_callback_array = (KevPOutputFunc*)kev_pargen_output_callback_array_c_cpp;
    func_group->output_symbol_array = (KevPOutputFunc*)kev_pargen_output_symbol_array_c_cpp;
    func_group->output_rule_info_array = (KevPOutputFunc*)kev_pargen_output_rule_info_array_c_cpp;
    func_group->output_state_symbol_mapping = (KevPOutputFunc*)kev_pargen_output_state_symbol_mapping_array_c_cpp;
  } else {
    kev_throw_error("output:", "unsupport language: ", language);
  }
}

void kev_pargen_output(const char* output_path, const char* tmpl_path, KevStringMap* env_var, KevFuncMap* funcs) {
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

static void kev_pargen_output_symbol_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  char** symbol_name = table_info->symbol_name;
  fprintf(out, "static const char* symbol_name[%d] = {\n", (int)table_info->goto_col_no);
  for (size_t i = 0; i < table_info->goto_col_no; ++i) {
    fputs("  \"", out);
    if (symbol_name[i])
      kev_pargen_output_escape_str(out, symbol_name[i]);
    else
      fputs("NULL", out);
    fputs("\",\n", out);
  }
  fprintf(out, "};\n\n");
}

static void kev_pargen_output_start_state_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  KevStringMapNode* node = kev_strmap_search(env_var, "state-type");
  const char* state_type = node ? node->value : "int16_t";
  fprintf(out, "static %s start_state = %d;\n\n", state_type, (int)table_info->start_state);
}

static void kev_pargen_output_action_table_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  size_t state_no = (size_t)table_info->state_no;
  size_t column_no = (size_t)table_info->action_col_no;
  KevPActionEntry** action = table_info->action_table;
  fprintf(out, "static ActionEntry action_tbl[%d][%d] = {", (int)state_no, (int)column_no);
  for (size_t i = 0; i < state_no; ++i) {
    fputs("\n  {", out);
    for (size_t j = 0; j < column_no; ++j) {
      if (j % 8 == 0) fputs( "\n    ", out);
      fprintf(out, "{ %d, %4d }, ", (int)action[i][j].action, (int)action[i][j].info);
    }
    fputs("\n  },", out);
  }
  fputs("\n};\n\n", out);
}

static void kev_pargen_output_goto_table_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  KevStringMapNode* node = kev_strmap_search(env_var, "state-type");
  const char* state_type = node ? node->value : "int16_t";
  size_t state_no = (size_t)table_info->state_no;
  size_t symbol_no = (size_t)table_info->goto_col_no;
  int** goto_tbl = table_info->goto_table;
  fprintf(out, "static %s goto_tbl[%d][%d] = {\n ", state_type, (int)state_no, (int)symbol_no);
  for (size_t i = 0; i < state_no; ++i) {
    fputs(" {", out);
    for (size_t j = 0; j < symbol_no; ++j) {
      if (j % 16 == 0) fputs( "\n    ", out);
      fprintf(out, "%4d,", (int)goto_tbl[i][j]);
    }
    fputs("\n  },", out);
  }
  fputs("\n};\n\n", out);
}

static void kev_pargen_output_rule_info_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  size_t rule_no = (size_t)table_info->rule_no;
  KevPRuleInfo* rules_info = table_info->rules_info;
  fprintf(out, "static RuleInfo rules_info[%d] = {", (int)rule_no);
  for (size_t i = 0; i < rule_no; ++i) {
    if (i % 8 == 0) fputs( "\n  ", out);
    fprintf(out, "{ %4d, %4d }, ", (int)rules_info[i].head_id, (int)rules_info[i].rulelen);
  }
  fputs("\n};\n\n", out);
}

static void kev_pargen_output_callback_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  size_t rule_no = (size_t)table_info->rule_no;
  KevPRuleInfo* rules_info = table_info->rules_info;
  KevStringMapNode* node = kev_strmap_search(env_var, "attr-idx-fmt");
  const char* attr_idx_fmt = node ? node->value : NULL;
  node = kev_strmap_search(env_var, "stk-idx-fmt");
  const char* stk_idx_fmt = node ? node->value : NULL;
  node = kev_strmap_search(env_var, "callback-head");
  const char* callback_head = node ? node->value : NULL;
  node = kev_strmap_search(env_var, "callback-tail");
  const char* callback_tail = node ? node->value : NULL;
  node = kev_strmap_search(env_var, "placeholder");
  char* placeholder = node ? node->value : NULL;

  for (size_t i = 0; i < rule_no; ++i) {
    if (rules_info[i].func_type == KEV_ACTFUNC_FUNCDEF) {
      if (!callback_head || !callback_tail || !placeholder) {
        kev_throw_error("output:", "environment variable 'attr-idx-fmt', 'callback-head', ", 
                        "'callback-head' and 'placeholder' should be defined");
      }
      char buf[(sizeof "_action_callback_" / sizeof "_action_callback_"[0]) + 30];
      size_t len = sprintf(buf, "_action_callback_%d", (int)i);
      kev_pargen_output_simple_replace(out, buf, buf + len, placeholder[0], callback_head);
      kev_pargen_output_action_code(out, rules_info[i].actfunc, placeholder[0], attr_idx_fmt, stk_idx_fmt);
      fputs(callback_tail, out);
    }
  }  

  fprintf(out, "static LRCallback* callbacks[%d] = {", (int)rule_no);
  for (size_t i = 0; i < rule_no; ++i) {
    if (rules_info[i].func_type == KEV_ACTFUNC_FUNCNAME) {
      fprintf(out, "\n  %s,", rules_info[i].actfunc ? rules_info[i].actfunc : "NULL");
    } else {
      fprintf(out, "\n  _action_callback_%d,", (int)i);
    }
  }
  fputs("\n};\n\n", out);
}

static void kev_pargen_output_state_symbol_mapping_array_c_cpp(FILE* out, KevPTableInfo* table_info, KevStringMap* env_var) {
  size_t state_no = table_info->state_no;
  int* mapping = table_info->state_to_symbol_id;
  fprintf(out, "static int state_symbol_mapping[%d] = {", (int)state_no);
  for (size_t i = 0; i < state_no; ++i) {
    if (i % 16 == 0) fputs("\n  ", out);
    fprintf(out, "%4d,", mapping[i]);
  }
  fputs("\n};\n\n", out);
}

static void kev_pargen_output_escape_str(FILE* out, const char* str) {
  for (const char* ptr = str; *ptr != '\0'; ++ptr) {
    switch (*ptr) {
      case '\n':
        fputs("\\n", out);
        break;
      case '\t':
        fputs("\\t", out);
        break;
      case '\\':
        fputs("\\\\", out);
        break;
      case '\r':
        fputs("\\r", out);
        break;
      case '\"':
        fputs("\\\"", out);
        break;
      case '\'':
        fputs("\\\'", out);
        break;
      default:
        fputc(*ptr, out);
        break;
    }
  }
}

void kev_pargen_output_action_code(FILE* out, const char* code, char placeholder, const char* attr_idx_fmt, const char* stk_idx_fmt) {
  for (const char* ptr = code; *ptr != '\0'; ++ptr) {
    if (*ptr == '$' || *ptr == '#') {
      char prefix = *ptr;
      const char* fmt = prefix == '$' ? attr_idx_fmt : stk_idx_fmt;
      ++ptr;
      if (*ptr == prefix) {
        fputc(prefix, out);
      } else if (*ptr >= '0' && *ptr <= '9') {
        const char* begin = ptr;
        while (isdigit(*++ptr)) continue;
        kev_pargen_output_simple_replace(out, begin, ptr, placeholder, fmt);
        --ptr;  /* ptr will increment in the for loop */
      } else if (isalpha(*ptr) || *ptr == '_') {
        const char* begin = ptr++;
        while (isalnum(*ptr) || *ptr == '_' || (uint8_t)*ptr > 0x7F) ++ptr;
        kev_pargen_output_simple_replace(out, begin, ptr, placeholder, fmt);
        --ptr;  /* ptr will increment in the for loop */
      } else {
        char endmark = *ptr == '(' ? ')' :
                       *ptr == '{' ? '}' :
                       *ptr == '[' ? ']' :
                       *ptr == '<' ? '>' :
                       *ptr;
        const char* begin = ptr + 1;
        while (*++ptr != endmark && *ptr != '\0') continue;
        kev_pargen_output_simple_replace(out, begin, ptr, placeholder, fmt);
      }
    } else {
      fputc(*ptr, out);
    }
  }
}

static void kev_pargen_output_simple_replace(FILE* out, const char* begin, const char* end, char placeholder, const char* fmt) {
  if (!fmt) kev_throw_error("output:", "attr-idx-fmt or stk-idx-fmt not defined", NULL);

  for (const char* ptr = fmt; *ptr != '\0'; ++ptr) {
    if (*ptr == placeholder) {
      for (const char* ptr = begin; ptr != end; ++ptr)
        fputc(*ptr, out);
    } else {
      fputc(*ptr, out);
    }
  }
}
