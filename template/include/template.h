#ifndef KEVCC_TEMPLATE_INCLUDE_TEMPLATE_H
#define KEVCC_TEMPLATE_INCLUDE_TEMPLATE_H

#include "utils/include/hashmap/str_map.h"
#include "utils/include/hashmap/func_map.h"

#include <stdio.h>


/* parse template file 'tmpl' and output result to 'output'. 'env_var' contains
 * the environment variables. 'funcs' contains functions. */
void kev_template_convert(FILE* output, FILE* tmpl, KevStringMap* env_var, KevFuncMap* funcs);


#endif
