#include "utils/include/os_spec/dir.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

char* kev_get_bin_dir(void) {
  char* buf = NULL;
  size_t size = 64;
#ifdef _WIN32
  size_t len = 0;
#else
  int len = 0;
#endif
  do {
    free(buf);
    size = size + size / 2;
    buf = (char*)malloc(sizeof (char) * size);
    if (!buf) return NULL;
#ifdef _WIN32
  } while ((len = (size_t)GetModuleFileNameA(NULL, buf, size)) == size);
#else
  } while ((len = readlink("/proc/self/exe",buf, size)) == -1);
#endif
  len--;
#ifdef _WIN32
  for (char* p = buf; *p != '\0'; ++p)
    if (*p == '\\') *p = '/';
#endif
  while (buf[--len] != '/') continue;
  buf[len + 1] = '\0';
  return buf;
}

