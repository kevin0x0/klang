#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H

#include "lexgen/include/lexgen/convert.h"
#include "lexgen/include/lexgen/options.h"

#include <stdio.h>

/* This structure is used to package a series of functions that are used to
 * convert the memory representation of transition tables and other information
 * into a text representation. */
typedef struct tagKevOutputFunc {
  void (*output_table)(FILE*, KevPatternBinary*);
  void (*output_pattern_mapping)(FILE*, KevPatternBinary*);
  void (*output_start)(FILE*, KevPatternBinary*);
  char* (*output_callback)(KevPatternBinary*);
  char* (*output_info)(KevPatternBinary*);
  char* (*output_macro)(KevPatternBinary*);
} KevOutputFunc;

/* fill the structure by language */
void kev_lexgen_output_set_func(KevOutputFunc* func_group, const char* language);
/* output to file */
void kev_lexgen_output_src(FILE* output, KevOptions* options, KevStringMap* env_var);
void kev_lexgen_output_help(void);

#endif
