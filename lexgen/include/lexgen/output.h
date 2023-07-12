#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OUTPUT_H

#include "lexgen/include/lexgen/options.h"
#include "lexgen/include/parser/parser.h"

#include <stdio.h>

void kev_lexgen_output_table(FILE* output, KevFA* dfa, size_t* pattern_mapping, char* language,
                             int charset, int length, size_t non_acc_no, size_t state_no);
void kev_lexgen_output_callback(FILE* output, char** callbacks, KevOptions* options, size_t arrlen);
void kev_lexgen_output_info(FILE* output, KevPatternList* list, size_t* pattern_mapping, KevOptions* options);
void kev_lexgen_output_src(FILE* output, KevOptions* options, KevStringMap* tmpl_map, char** callbacks, size_t arrlen);
void kev_lexgen_output_arc(char* compiler, KevOptions* options);
void kev_lexgen_output_sha(char* compiler, KevOptions* options);
void kev_lexgen_output_help(void);

#endif
