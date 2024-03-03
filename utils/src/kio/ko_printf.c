#include "utils/include/kio/ko.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int ko_printf(Ko* ko, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = ko_vprintf(ko, fmt, ap);
  va_end(ap);
  return n;
}


int ko_vprintf(Ko* ko, const char* fmt, va_list arglist) {
  int bufsize = 128;
  char* buffer = (char*)malloc(bufsize);
  if (!buffer) return -1;

  va_list ap;
  va_copy(ap, arglist);
  int len = vsnprintf(buffer, bufsize, fmt, ap);
  if (len < 0) {
    free(buffer);
    return len;
  }
  if (len >= bufsize) {
    free(buffer);
    bufsize = len + 1;
    buffer = (char*)malloc(bufsize);
    if (!buffer) return -1;
    va_copy(ap, arglist);
    len = vsnprintf(buffer, bufsize, fmt, ap);
  }
  if (len < 0) {
    free(buffer);
    return len;
  }
  int writesize = ko_write(ko, buffer, len);
  free(buffer);
  return writesize;
}
