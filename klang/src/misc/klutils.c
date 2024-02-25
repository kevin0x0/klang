#include "klang/include/misc/klutils.h"
#include <stdio.h>
#include <stdlib.h>

void kl_abort(const char* expr, int line_no, const char *filename, const char *info) {
  fprintf(stderr, "\nassertion failed: %s:%d:%s\n%s\n", filename, line_no, expr, info);
  exit(EXIT_FAILURE);
}
