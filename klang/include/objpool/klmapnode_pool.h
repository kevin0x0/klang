#ifndef KEVCC_KLANG_INCLUDE_OBJPOOL_KLMAPNODE_POOL_H
#define KEVCC_KLANG_INCLUDE_OBJPOOL_KLMAPNODE_POOL_H

#include "klang/include/value/klmap.h"

KlMapNode* klmapnode_pool_allocate(void);
void klmapnode_pool_deallocate(KlMapNode* node);
void klmapnode_pool_free(void);

#endif
