#ifndef KEVCC_UTILS_INCLUDE_DIR_H
#define KEVCC_UTILS_INCLUDE_DIR_H

/* Due to platform-specificity, this file defines some wrapper functions
 * related to directory operations. */

char* kev_getcwd(void);
char* kev_get_exe_dir(void);
char* kev_get_kevcc_dir(void);
char* kev_get_lexgen_resources_dir(void);
char* kev_get_lexgen_tmp_dir(void);
#endif
