#include "klang/include/code/klcontbl.h"
#include "klang/include/code/klgen.h"
#include "klang/include/code/klsymtbl.h"
#include "klang/include/code/klcodeval.h"
#include "klang/include/cst/klcst.h"
#include "klang/include/cst/klcst_expr.h"
#include "klang/include/value/klvalue.h"
#include "klang/include/vm/klinst.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>





/* error handler */
size_t klgen_helper_locateline(Ki* input, size_t offset);
bool klgen_helper_showline_withcurl(KlGenUnit* parser, Ki* input, KlFileOffset begin, KlFileOffset end);

void klgen_error(KlGenUnit* gen, KlFileOffset begin, KlFileOffset end, const char* format, ...) {
  va_list args;
  va_start(args, format);
  klerror_errorv(gen->klerror, gen->input, gen->config.inputname, begin, end, format, args);
  va_end(args);
}
