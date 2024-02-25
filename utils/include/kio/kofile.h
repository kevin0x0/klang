#ifndef KEVCC_UTILS_INCLUDE_KIO_KOFILE_H
#define KEVCC_UTILS_INCLUDE_KIO_KOFILE_H

#include "utils/include/kio/ko.h"
#include <stdio.h>


Ko* kofile_create(const char* filepath);
Ko* kofile_attach(FILE* file);



#endif
