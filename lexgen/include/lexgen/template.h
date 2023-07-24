#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_TEMPLATE_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_TEMPLATE_H

#include "lexgen/include/lexgen/options.h"
#include "lexgen/include/parser/hashmap/str_map.h"

#include <stdio.h>

/* parse template file 'tmpl' and output result to 'output'
 * 'env_var' store the environment variables. */
void kev_template_convert(FILE* output, FILE* tmpl, KevStringMap* env_var);

#endif
