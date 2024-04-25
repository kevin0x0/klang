#include "klang/include/misc/klutils.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG

void kl_abort(const char* expr, const char* head, int line_no, const char *filename, const char *info) {
  fprintf(stderr, "\n%s: in %s:%d: %s\n%s\n", head, filename, line_no, expr, info);
  exit(EXIT_FAILURE);
}

#endif
