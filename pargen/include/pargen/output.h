#ifndef KEVCC_PARGEN_INCLUDE_PARGEN_OUTPUT_H
#define KEVCC_PARGEN_INCLUDE_PARGEN_OUTPUT_H

#include "pargen/include/pargen/options.h"
#include "kevlr/include/collection.h"
#include "kevlr/include/table.h"
#include "utils/include/hashmap/str_map.h"
#include "utils/include/hashmap/func_map.h"

#include <stdio.h>

typedef void KevPOutputFunc(FILE*, void*, KevStringMap*);
/* This structure is used to package a series of functions that are used to
 * convert the memory representation of transition tables and other information
 * into a text representation. */
typedef struct tagKevPOutputFuncGroup {
  KevPOutputFunc* output_action_table;
  KevPOutputFunc* output_goto_table;
  KevPOutputFunc* output_start_state;
  KevPOutputFunc* output_rule_info_array;
  KevPOutputFunc* output_callback_array;
  KevPOutputFunc* output_symbol_array;
} KevPOutputFuncGroup;

/* fill the structure by language */
void kev_pargen_output_set_func(KevPOutputFuncGroup* func_group, const char* language);
/* output to file */
void kev_pargen_output(const char* output_path, const char* tmpl_path, KevStringMap* env_var, KevFuncMap* funcs);

void kev_pargen_output_help(void);
void kev_pargen_output_lrinfo(const char* collecinfo_path, const char* actioninfo_path,
                              const char* gotoinfo_path, const char* symbolinfo_path,
                              KevLRCollection* collec, KevLRTable* table);

void kev_pargen_output_action_code(FILE* out, const char* code, char placeholder, const char* attr_idx_fmt, const char* stk_idx_fmt);

#endif
