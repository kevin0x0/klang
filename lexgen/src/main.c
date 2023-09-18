#include "lexgen/include/lexgen/cmdline.h"
#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/lexgen/options.h"

int main(int argc, char **argv) {
  KevLOptions options;
  kev_lexgen_get_options(argc, argv, &options);
  kev_lexgen_control(&options);
  kev_lexgen_destroy_options(&options);
  return 0;
}
