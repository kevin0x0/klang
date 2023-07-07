#include "lexgen/include/lexgen/options.h"
#include "lexgen/include/lexgen/control.h"
#include "lexgen/include/lexgen/cmdline.h"


int main(int argc, char** argv) {
  KevOptions options;
  kev_lexgen_get_options(argc, argv, &options);
  kev_lexgen_control(&options);
  kev_lexgen_destroy_options(&options);
  return 0;
}
