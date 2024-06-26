#ifndef _KLANG_INCLUDE_MISC_KLUTILS_H_
#define _KLANG_INCLUDE_MISC_KLUTILS_H_

#include <stdint.h>

#define kl_likely(expr)       (__builtin_expect(!!(expr), 1))
#define kl_unlikely(expr)     (__builtin_expect(!!(expr), 0))

#define kl_noreturn           _Noreturn


#define klcast(totype, obj)   ((totype)(obj))
#define klbit(idx)            ((uintmax_t)1 << (idx))
#define kllowbits(a, n)       (a & (klbit(n) - 1))
#define kllow4bits(a)         kllowbits(a, 4)
#define kllow8bits(a)         kllowbits(a, 8)


#define kl_static_assert(expr, info)    _Static_assert((expr), info)
#define kl_unused(param)                ((void)(param))


#define KL_DERIVE_FROM(base, prefix)    KL_DERIVE_FROM_##base(prefix)

#ifdef NDEBUG
#define kl_assert(expr, info)   ((void)0)
#define kltodo(message)         ((void)0)
#define kl_debug(decl, expr)    ((void)0)
#else
kl_noreturn void kl_abort(const char* expr, const char* head, int line_no, const char* filename, const char* info);

#define kl_assert(expr, info)           ((void)((expr) || (kl_abort(#expr, "assertion failed", __LINE__, __FILE__, (info)), 0)))
#define kltodo(message)                 kl_abort(message, "TODO", __LINE__, __FILE__, "")
#define kl_debug(decl, expr)            decl = (expr)

#endif

#if defined (__GNUC__) || defined (__clang__)
#define KL_FALLTHROUGH  __attribute__ ((fallthrough))
#else
#define KL_FALLTHROUGH  /* fall through */
#endif


#endif
