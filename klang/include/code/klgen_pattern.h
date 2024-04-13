/* code generator for pattern match and pattern extract */

#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#include "klang/include/code/klgen.h"


size_t klgen_pattern_deconstruct(KlGenUnit* gen, KlCst* pattern, size_t targettail);
bool klgen_pattern_fastdeconstruct(KlGenUnit* gen, KlCst* pattern);
size_t klgen_pattern_assign_stkid(KlGenUnit* gen, KlCst* pattern, size_t base);

void klgen_pattern_newsymbol(KlGenUnit* gen, KlCst* pattern);
static inline void klgen_patterns_newsymbol(KlGenUnit* gen, KlCst** patterns, size_t npattern);
static inline size_t klgen_patterns_assign_stkid(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base);


static inline size_t klgen_patterns_assign_stkid(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base) {
  size_t nid = 0;
  for (size_t i = 0; i < npattern; ++i)
    nid += klgen_pattern_assign_stkid(gen, patterns[i], base + nid);
  return nid;
}

static inline void klgen_patterns_newsymbol(KlGenUnit* gen, KlCst** patterns, size_t npattern) {
  for (size_t i = 0; i < npattern; ++i)
    klgen_pattern_newsymbol(gen, patterns[i]);
}

#endif
