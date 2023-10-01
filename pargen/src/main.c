#include "pargen/include/pargen/cmdline.h"
#include "pargen/include/pargen/control.h"

int main(int argc, char** argv) {
  KevPOptions options;
  kev_pargen_get_options(argc, argv, &options);
  kev_pargen_control(&options);
  kev_pargen_destroy_options(&options);
  return 0;
}
