#ifndef KEVCC_UTILS_INCLUDE_UTILS_UTILS_H
#define KEVCC_UTILS_INCLUDE_UTILS_UTILS_H

#define k_likely(expr)    (__builtin_expect(!!(expr), 1))
#define k_unlikely(expr)  (__builtin_expect(!!(expr), 0))


#endif
