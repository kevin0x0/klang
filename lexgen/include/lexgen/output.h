#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H

#include "lexgen/include/lexgen/convert.h"
#include "lexgen/include/lexgen/options.h"
#include "utils/include/hashmap/strx_map.h"

#include <stdio.h>

typedef void KevOutputFunc(FILE*, void*);

/* This structure is used to package a series of functions that are used to
 * convert the memory representation of transition tables and other information
 * into a text representation. */
typedef struct tagKevOutputFuncGroup {
  KevOutputFunc* output_table;
  KevOutputFunc* output_pattern_mapping;
  KevOutputFunc* output_start;
  KevOutputFunc* output_callback;
  KevOutputFunc* output_info;
  KevOutputFunc* output_macro;
} KevOutputFuncGroup;

/* fill the structure by language */
void kev_lexgen_output_set_func(KevOutputFuncGroup* func_group, const char* language);
/* output to file */
void kev_lexgen_output_src(FILE* output, KevOptions* options, KevStringMap* env_var, KevStrXMap* funcs);
void kev_lexgen_output_help(void);

#endif
