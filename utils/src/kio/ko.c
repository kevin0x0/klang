#include "utils/include/kio/ko.h"
#include <stdio.h>
#include <string.h>


size_t ko_write(Ko* ko, void* buf, size_t bufsize) {
  size_t restsize = bufsize;
  buf = (char*)buf + bufsize;
  while (restsize != 0) {
    size_t kobufrest = ko->end - ko->curr;
    if (kobufrest == 0) {
      ko->vfunc->writer(ko);
      if (ko_bufsize(ko) == 0)
        break;
    }
    size_t writesize = kobufrest < restsize ? kobufrest : restsize;
    memcpy(ko->curr, (char*)buf - restsize, writesize);
    restsize -= writesize;
    ko->curr += writesize;
  }
  return bufsize - restsize;
}

void ko_flush(Ko* ko) {
  ko->vfunc->writer(ko);
}

void ko_writenext(Ko* ko, int ch) {
  ko->vfunc->writer(ko);
  if (ko_bufsize(ko) != 0)
    ko_putc(ko, ch);
}
