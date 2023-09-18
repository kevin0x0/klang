#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_CMDLINE_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_CMDLINE_H

#include "lexgen/include/lexgen/options.h"

/* set options from command line parameters */
void kev_lexgen_get_options(int argc, char** argv, KevLOptions* options);
/* free resources */
void kev_lexgen_destroy_options(KevLOptions* options);

#endif
