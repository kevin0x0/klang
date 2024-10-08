#ifndef _KLANG_INCLUDE_CODE_KLGEN_PATTERN_H_
#define _KLANG_INCLUDE_CODE_KLGEN_PATTERN_H_

#include "include/code/klcodeval.h"
#include "include/code/klgen.h"


KlCStkId klgen_pattern_binding(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target);
KlCStkId klgen_pattern_matching(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId target);
/* deconstruct a val to top of stack. */
void klgen_pattern_binding_tostktop(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId val);
void klgen_pattern_matching_tostktop(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId val);
size_t klgen_pattern_count_result(KlGenUnit* gen, KlAstExpr* pattern);
bool klgen_pattern_fastbinding(KlGenUnit* gen, KlAstExpr* pattern);
bool klgen_pattern_fastmatching(KlGenUnit* gen, KlAstExpr* pattern);

KlCStkId klgen_pattern_newsymbol(KlGenUnit* gen, KlAstExpr* pattern, KlCStkId base);
void klgen_pattern_do_assignment(KlGenUnit* gen, KlAstExpr* pattern);
static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern);
static inline KlCStkId klgen_patterns_newsymbol(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern, KlCStkId base);
static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern);

static inline void klgen_patterns_do_assignment(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern) {
  for (size_t i = npattern; i-- > 0;)
    klgen_pattern_do_assignment(gen, patterns[i]);
}

static inline size_t klgen_patterns_count_result(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern) {
  size_t nid = 0;
  for (size_t i = 0; i < npattern; ++i)
    nid += klgen_pattern_count_result(gen, patterns[i]);
  return nid;
}

static inline KlCStkId klgen_patterns_newsymbol(KlGenUnit* gen, KlAstExpr** patterns, size_t npattern, KlCStkId base) {
  for (size_t i = 0; i < npattern; ++i)
    base = klgen_pattern_newsymbol(gen, patterns[i], base);
  return base;
}

#endif
