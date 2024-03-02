#include "klang/include/misc/klutils.h"
#include <stdio.h>
#include <stdlib.h>

void kl_abort(const char* expr, int line_no, const char *filename, const char *info) {
  fprintf(stderr, "\nassertion failed: %s in %s:%d\n%s\n", expr, filename, line_no, info);
  exit(EXIT_FAILURE);
}
