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

bool TokenBucket::consume (int bytes, int rate, int burstSize)
{
    uint64_t tsNow = MicrosecondsSinceEpoch ();

    uint32_t tokens = burstSize;

    if (m_tsLast)
    {
	int tsDiff = tsNow - m_tsLast;
	tokens = std::min ((tsDiff * rate) / 1000000, burstSize);
    }

    if (tokens < bytes)
    {
	return false;
    }

    tokens -= bytes;
    m_tsLast = tsNow - ((tokens * 1000000) / rate);
    return true;
}
