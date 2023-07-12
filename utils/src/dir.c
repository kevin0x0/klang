#include "utils/include/dir.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

char* kev_getcwd(void) {
  exit(EXIT_FAILURE);
  return NULL;
}

char* kev_get_bin_dir(void) {
  char* buf = NULL;
  size_t size = 128;
  size_t len = 0;
  do {
    free(buf);
    buf = (char*)malloc(sizeof (char) * size);
    if (!buf) return NULL;
    size = size + size / 2;
#ifdef _WIN32
  } while ((len = GetModuleFileNameA(NULL, buf, size)) == -1);
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

char* kev_get_kevcc_dir(void) {
  exit(EXIT_FAILURE);
  return NULL;
}

char* kev_get_lexgen_resources_dir(void) {
  char* dir = kev_get_bin_dir();
  if (!dir) return NULL;
  size_t len = strlen(dir);
  char* res_dir = (char*)realloc(dir, sizeof (char) * (len + 20));
  if (!res_dir) {
    free(dir);
    return NULL;
  }
  res_dir[len - 4] = '\0';
  strcat(res_dir, "resources/");
  return res_dir;
}

char* kev_get_lexgen_tmp_dir(void) {
  char* dir = kev_get_bin_dir();
  if (!dir) return NULL;
  size_t len = strlen(dir);
  char* res_dir = (char*)realloc(dir, sizeof (char) * (len + 20));
  if (!res_dir) {
    free(dir);
    return NULL;
  }
  res_dir[len - 4] = '\0';
  strcat(res_dir, "tmp/");
  return res_dir;
}

