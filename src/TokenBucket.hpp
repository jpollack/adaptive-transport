#pragma once
#include <stdint.h>

class TokenBucket
{
public:
    TokenBucket ();
    ~TokenBucket ();

    bool consume (int bytes, int rate, int burstSize);
    
private:
    uint64_t m_tsLast;
};
