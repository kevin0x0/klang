#ifndef KEVCC_UTILS_INCLUDE_OS_SPEC_DIR_H
#define KEVCC_UTILS_INCLUDE_OS_SPEC_DIR_H

#include <stdbool.h>

/* Due to platform-specificity, this file defines some wrapper functions
 * related to directory operations. */

char* kev_get_bin_dir(void);
char* kev_get_relpath(const char* from, const char* to);
char* kev_trunc_leaf(const char* path);
bool kev_is_relative(const char* path);

#endif
