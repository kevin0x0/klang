#include "utils/include/kio/ki.h"
#include <stdio.h>
#include <string.h>


int ki_loadnext(Ki* ki) {
  ki->vfunc->reader(ki);
  if (ki_bufsize(ki) != 0)
    return ki_getc(ki);
  return KOF;
}

size_t ki_read(Ki* ki, void* buf, size_t bufsize) {
  size_t restsize = bufsize;
  buf = (char*)buf + bufsize;
  while (restsize != 0) {
    size_t kibufrest = ki->end - ki->curr;
    if (kibufrest == 0) {
      ki->vfunc->reader(ki);
      if (ki_bufsize(ki) == 0)
        break;
    }
    size_t readsize = restsize < kibufrest ? restsize : kibufrest;
    memcpy((char*)buf - restsize, ki->curr, readsize);
    ki->curr += readsize;
    restsize -= readsize;
  }
  return bufsize - restsize;
}
