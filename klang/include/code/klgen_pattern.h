/* code generator for pattern match and pattern extract */

#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#include "klang/include/code/klgen.h"


size_t klgen_pattern_extract(KlGenUnit* gen, KlCst* pattern, size_t targettail);
size_t klgen_pattern_assign_stkid(KlGenUnit* gen, KlCst* pattern, size_t base);
void klgen_pattern_newsymbol(KlGenUnit* gen, KlCst* pattern);
static inline size_t klgen_patterns_assign_stkid(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base);

static inline size_t klgen_patterns_assign_stkid(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base) {
  size_t nid = 0;
  for (size_t i = 0; i < npattern; ++i)
    nid += klgen_pattern_assign_stkid(gen, patterns[i], base + nid);
  return nid;
}

#endif
