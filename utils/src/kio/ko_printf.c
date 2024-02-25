#include "utils/include/kio/ko.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

int ko_printf(Ko* ko, const char* fmt, ...) {
  va_list list;
  va_start(list, fmt);
  int bufsize = 128;
  char* buffer = (char*)malloc(bufsize);
  if (!buffer) return -1;

  int len = vsnprintf(buffer, bufsize, fmt, list);
  if (len < 0) {
    free(buffer);
    return len;
  }
  if (len >= bufsize) {
    free(buffer);
    bufsize = len + 1;
    buffer = (char*)malloc(bufsize);
    if (!buffer) return -1;
    len = vsnprintf(buffer, bufsize, fmt, list);
  }
  va_end(list);
  if (len < 0) {
    free(buffer);
    return len;
  }
  int writesize = ko_write(ko, buffer, len);
  free(buffer);
  return writesize;
}
