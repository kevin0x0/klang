#ifndef KEVCC_UTILS_INCLUDE_BITS_BITOP_H
#define KEVCC_UTILS_INCLUDE_BITS_BITOP_H

#if defined (__GNUC__) || defined (__clang__)
#define kbit_ctz(x)         (__builtin_ctzll(x))
#define kbit_popcount(x)    (__builtin_popcountll(x))
#else
#error "TODO: give software implementation for bit operation"
#endif

#endif
