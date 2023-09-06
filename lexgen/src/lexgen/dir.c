#include "lexgen/include/lexgen/dir.h"

#include <stdlib.h>
#include <string.h>

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
