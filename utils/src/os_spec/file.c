#include "utils/include/os_spec/file.h"

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>

size_t kev_file_size(FILE* file) {
  HANDLE hfile = (HANDLE)_get_osfhandle(_fileno(file));
  LARGE_INTEGER li;
  GetFileSizeEx(hfile, &li);
  size_t size = li.QuadPart;
  return size;
}

#else
#include <unistd.h>
#include <sys/stat.h>

size_t kev_file_size(FILE* file) {
  int fd = fileno(file);
  struct stat file_state;
  fstat(fd, &file_state);
  return file_state.st_size;
}

#endif
