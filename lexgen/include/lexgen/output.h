#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H

#include "lexgen/include/lexgen/convert.h"
#include "lexgen/include/lexgen/options.h"
#include "utils/include/hashmap/func_map.h"

#include <stdio.h>

typedef void KevLOutputFunc(FILE*, void*, KevStringMap*);

/* This structure is used to package a series of functions that are used to
 * convert the memory representation of transition tables and other information
 * into a text representation. */
typedef struct tagKevLOutputFuncGroup {
  KevLOutputFunc* output_table;
  KevLOutputFunc* output_pattern_mapping;
  KevLOutputFunc* output_start;
  KevLOutputFunc* output_callback;
  KevLOutputFunc* output_info;
  KevLOutputFunc* output_macro;
} KevLOutputFuncGroup;

/* fill the structure by language */
void kev_lexgen_output_set_func(KevLOutputFuncGroup* func_group, const char* language);
/* output to file */
void kev_lexgen_output(const char* output_path, const char* tmpl_path, KevStringMap* env_var, KevFuncMap* funcs);
void kev_lexgen_output_help(void);

#endif
