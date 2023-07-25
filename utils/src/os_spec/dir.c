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
  size_t len = 0;
  do {
    free(buf);
    size = size + size / 2;
    buf = (char*)malloc(sizeof (char) * size);
    if (!buf) return NULL;
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

char* kev_get_relpath(char* from, char* to) {
  size_t i = 0;
  while (from[i] == to[i] && from[i] != '\0')
    i++;
  while (i != 0 && from[--i] != '/' && from[i] != '\\')
    continue;
  if (i != 0) ++i;
  size_t dir_depth = 0;
  for (int j = i; from[j] != '\0'; ++j) {
    if (from[j] == '/' || from[j] == '\\')
      dir_depth++;
  }
  char* relpath = (char*)malloc(sizeof (char) * (dir_depth * 3 + strlen(to) + 1));
  if (!relpath) return NULL;
  for (int j = 0; j < dir_depth; ++j) {
    relpath[j * 3] = '.';
    relpath[j * 3 + 1] = '.';
    relpath[j * 3 + 2] = '/';
  }
  relpath[dir_depth * 3] = '\0';
  strcat(relpath, to + i);
  return relpath;
}

char* kev_trunc_leaf(char* path) {
  int i = 0;
  char* p = path - 1;
  while (*++p != '\0') {
    if (*p == '/' || *p == '\\')
      i = p - path;
  }
  path[i + 1] = '\0';
  return path;
}
