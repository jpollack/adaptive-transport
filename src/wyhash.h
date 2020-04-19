#pragma once
// see:
// https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/


#include <stdint.h>

uint64_t wyhash64_x; 

static inline uint64_t wyhash64_stateless (uint64_t *seed)
{
    *seed += (uint64_t)0x60bee2bee120fc15;
    __uint128_t tmp;
    tmp = (__uint128_t) *seed * (uint64_t)0xa3b195354a39b70d;
    uint64_t m1 = (tmp >> 64) ^ tmp;
    tmp = (__uint128_t)m1 * (uint64_t) 0x1b03738712fad5c9;
    uint64_t m2 = (tmp >> 64) ^ tmp;
    return m2;
}

static inline void wyhash64_seed(uint64_t seed) { wyhash64_x = seed; }

static inline uint64_t wyhash64(void) { return wyhash64_stateless(&wyhash64_x); }

