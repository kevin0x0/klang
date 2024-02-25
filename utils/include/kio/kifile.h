#ifndef KEVCC_UTILS_INCLUDE_KIO_KIFILE_H
#define KEVCC_UTILS_INCLUDE_KIO_KIFILE_H

#include "utils/include/kio/ki.h"
#include <stdio.h>


Ki* kifile_create(const char* filepath);
Ki* kifile_attach(FILE* file);

#endif
