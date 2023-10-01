#ifndef KEVCC_PARGEN_INCLUDE_PARGEN_CMDLINE_H
#define KEVCC_PARGEN_INCLUDE_PARGEN_CMDLINE_H

#include "pargen/include/pargen/options.h"

/* set options from command line parameters */
void kev_pargen_get_options(int argc, char** argv, KevPOptions* options);
/* free resources */
void kev_pargen_destroy_options(KevPOptions* options);

#endif
