#include "lexgen/include/lexgen/error.h"

#include <stdio.h>
#include <stdlib.h>

void kev_throw_error(const char* header, const char* info, const char* additional_info) {
  fprintf(stderr, "%s %s", header, info);
  if (additional_info)
    fputs(additional_info, stderr);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}
