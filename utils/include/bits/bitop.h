#ifndef KEVCC_UTILS_INCLUDE_BITS_BITOP_H
#define KEVCC_UTILS_INCLUDE_BITS_BITOP_H

#define kbit_ctz64(x)       (__builtin_ctzll(x))
#define kbit_popcount64(x)  (__builtin_popcountll(x))

#endif
