#include "pargen/include/pargen/output.h"

int main(int argc, char** argv) {
  char fmt[] = "stk[\xFF]";
  kev_pargen_output_action_code(stdout, "$(jijdi + jiwj) = $1 + $2;", '\xFF', fmt, fmt);
  return 0;
}
