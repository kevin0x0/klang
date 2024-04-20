/* code generator for pattern match and pattern extract */

#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#include "klang/include/code/klgen.h"


size_t klgen_pattern_deconstruct(KlGenUnit* gen, KlCst* pattern, size_t target);
/* deconstruct a val to top of stack.
 * do assign id and newsymbol. */
size_t klgen_pattern_deconstruct_tostktop(KlGenUnit* gen, KlCst* pattern, size_t val);
size_t klgen_pattern_count_result(KlGenUnit* gen, KlCst* pattern);
bool klgen_pattern_fastdeconstruct(KlGenUnit* gen, KlCst* pattern);

size_t klgen_pattern_newsymbol(KlGenUnit* gen, KlCst* pattern, size_t base);
void klgen_pattern_do_assignment(KlGenUnit* gen, KlCst* pattern);
static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlCst** patterns, size_t npattern);
static inline size_t klgen_patterns_newsymbol(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base);
static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlCst** patterns, size_t npattern);

static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlCst** patterns, size_t npattern) {
  for (size_t i = npattern; i-- > 0;)
    klgen_pattern_do_assignment(gen, patterns[i]);
}

static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlCst** patterns, size_t npattern) {
  size_t nid = 0;
  for (size_t i = 0; i < npattern; ++i)
    nid += klgen_pattern_count_result(gen, patterns[i]);
  return nid;
}

static inline size_t klgen_patterns_newsymbol(KlGenUnit* gen, KlCst** patterns, size_t npattern, size_t base) {
  for (size_t i = 0; i < npattern; ++i)
    base = klgen_pattern_newsymbol(gen, patterns[i], base);
  return base;
}

#endif
