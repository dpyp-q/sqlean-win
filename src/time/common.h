// Copyright (c) 2024 Anton Zhiyanov, MIT License
// https://github.com/nalgeon/sqlean

// Based on Go's time package, BSD 3-Clause License
// https://github.com/golang/go

// Conversions shared by the time and duration methods.

#ifndef TIME_COMMON_H
#define TIME_COMMON_H

#include <stdint.h>

// uint64_to_int64 interprets u modulo 2^64 as a signed 64-bit value.
// Unlike a direct cast, this is fully defined when u > INT64_MAX.
static inline int64_t uint64_to_int64(uint64_t u) {
    if (u <= INT64_MAX) {
        return (int64_t)u;
    }
    return -1 - (int64_t)(UINT64_MAX - u);
}

#endif /* TIME_COMMON_H */
