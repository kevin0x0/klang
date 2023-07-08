#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_CMDLINE_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_CMDLINE_H

#include "lexgen/include/lexgen/options.h"

void kev_lexgen_get_options(int argc, char** argv, KevOptions* options);
void kev_lexgen_destroy_options(KevOptions* options);

#endif
