/* code generator for pattern matching and pattern binding */

#ifndef KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#define KEVCC_KLANG_INCLUDE_CODE_KLGEN_PATTERN_H
#include "include/code/klgen.h"


size_t klgen_pattern_binding(KlGenUnit* gen, KlAst* pattern, size_t target);
void klgen_pattern_fastmatching(KlGenUnit* gen, KlAst* pattern);
void klgen_pattern_matching(KlGenUnit* gen, KlAst* pattern, size_t target);
/* deconstruct a val to top of stack. */
void klgen_pattern_binding_tostktop(KlGenUnit* gen, KlAst* pattern, size_t val);
size_t klgen_pattern_count_result(KlGenUnit* gen, KlAst* pattern);
bool klgen_pattern_fastbinding(KlGenUnit* gen, KlAst* pattern);

size_t klgen_pattern_newsymbol(KlGenUnit* gen, KlAst* pattern, size_t base);
void klgen_pattern_do_assignment(KlGenUnit* gen, KlAst* pattern);
static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlAst** patterns, size_t npattern);
static inline size_t klgen_patterns_newsymbol(KlGenUnit* gen, KlAst** patterns, size_t npattern, size_t base);
static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlAst** patterns, size_t npattern);

static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlAst** patterns, size_t npattern) {
  for (size_t i = npattern; i-- > 0;)
    klgen_pattern_do_assignment(gen, patterns[i]);
}

static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlAst** patterns, size_t npattern) {
  size_t nid = 0;
  for (size_t i = 0; i < npattern; ++i)
    nid += klgen_pattern_count_result(gen, patterns[i]);
  return nid;
}

static inline size_t klgen_patterns_newsymbol(KlGenUnit* gen, KlAst** patterns, size_t npattern, size_t base) {
  for (size_t i = 0; i < npattern; ++i)
    base = klgen_pattern_newsymbol(gen, patterns[i], base);
  return base;
}

#endif
