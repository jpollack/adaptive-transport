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

uint64_t MicrosecondsSinceEpoch (void);
uint64_t wait_until (uint64_t when);

