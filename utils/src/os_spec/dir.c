#include "utils/include/os_spec/dir.h"
#include "utils/include/string/kev_string.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

char* kev_get_bin_dir(void) {
  char* buf = NULL;
  size_t buf_size = 64;
#ifdef _WIN32
  size_t len = 0;
#else
  int len = 0;
#endif
  do {
    free(buf);
    buf_size = buf_size + buf_size / 2;
    buf = (char*)malloc(sizeof (char) * buf_size);
    if (!buf) return NULL;
#ifdef _WIN32
  } while ((len = (size_t)GetModuleFileNameA(NULL, buf, buf_size)) == buf_size);
#else
  } while ((len = readlink("/proc/self/exe",buf, buf_size)) == -1);
#endif
  len--;
#ifdef _WIN32
  for (char* ptr = buf; *ptr != '\0'; ++ptr)
    if (*ptr == '\\') *ptr = '/';
#endif
  while (buf[--len] != '/') continue;
  buf[len + 1] = '\0';
  return buf;
}

char* kev_get_relpath(const char* from, const char* to) {
  size_t i = 0;
  while (from[i] == to[i] && from[i] != '\0')
    i++;
  while (i != 0 && from[--i] != '/' && from[i] != '\\')
    continue;
  if (i != 0) ++i;
  size_t dir_depth = 0;
  for (size_t j = i; from[j] != '\0'; ++j) {
    if (from[j] == '/' || from[j] == '\\')
      dir_depth++;
  }
  char* relpath = (char*)malloc(sizeof (char) * (dir_depth * 3 + strlen(to) + 1));
  if (!relpath) return NULL;
  for (size_t j = 0; j < dir_depth; ++j) {
    relpath[j * 3] = '.';
    relpath[j * 3 + 1] = '.';
    relpath[j * 3 + 2] = '/';
  }
  relpath[dir_depth * 3] = '\0';
  strcat(relpath, to + i);
  return relpath;
}

char* kev_trunc_leaf(const char* path) {
  size_t i = 0;
  char* cp_path = kev_str_copy(path);
  if (!cp_path) return NULL;
  char* p = cp_path - 1;
  while (*++p != '\0') {
    if (*p == '/' || *p == '\\')
      i = p - cp_path;
  }
  cp_path[i + 1] = '\0';
  return cp_path;
}

bool kev_is_relative(const char* path) {
#ifdef _WIN32
  return !isupper(path[0]) || path[1] != ':';
#else
  return path[0] != '/';
#endif
}
