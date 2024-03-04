#ifndef KEVCC_KLANG_INCLUDE_MISC_KLUTILS_H
#define KEVCC_KLANG_INCLUDE_MISC_KLUTILS_H

#include <stdbool.h>

#define kl_likely(expr)       (__builtin_expect(!!(expr), 1))
#define kl_unlikely(expr)     (__builtin_expect(!!(expr), 0))


#define klcast(totype, obj)   ((totype)(obj))
#define klbit(idx)            ((size_t)1 << (idx))
#define kllowbits(a, n)       (a & (klbit(n) - 1))
#define kllow4bits(a)         kllowbits(a, 4)
#define kllow8bits(a)         kllowbits(a, 8)



#ifdef NDEBUG
#define kl_assert(expr, info)   ((void)0)
#else
#define kl_assert(expr, info)   ((void)((expr) || (kl_abort(#expr, __LINE__, __FILE__, (info)), 0)))
void kl_abort(const char* expr, int line_no, const char* filename, const char* info);
#endif


#endif
