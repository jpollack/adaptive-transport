#include "TokenBucket.hpp"
#include "Timing.hpp"
#include <algorithm>

TokenBucket::TokenBucket ()
    : m_tsLast (0)
{
    
}

TokenBucket::~TokenBucket ()
{
    
}
// TODO:  have a convenience method to approximate the amount of time to sleep
// for.
bool TokenBucket::consume (int bytes, int rate, int burstSize)
{
    uint64_t tsNow = MicrosecondsSinceEpoch ();
    uint64_t tsDiff = tsNow - m_tsLast;
    double dtokens = std::min ((double)burstSize, ((double)tsDiff * (double)rate) / 1000000.0);
    uint32_t tokens = m_tsLast ? (uint32_t)dtokens : burstSize;
    if (tokens < bytes)
    {
	return false;
    }

    tokens -= bytes;
    uint64_t t0 = ((uint64_t)tokens * (uint64_t)1000000) / rate;
    m_tsLast = tsNow - t0;
    return true;

}
