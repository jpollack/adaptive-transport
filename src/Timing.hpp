#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <stdint.h>
#include <tuple>
#include <numeric>
#include <cmath>

void TSC_MarkStart (uint64_t &ret);
void TSC_MarkStop (uint64_t &ret);
void TSC_MarkMid (uint64_t &t0, uint64_t &t1);

inline uint64_t MicrosecondsSinceEpoch(void) 
{
    return std::chrono::duration_cast<std::chrono::microseconds>
              (std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t wait_until (uint64_t when);

