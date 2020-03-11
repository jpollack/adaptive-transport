#include "BWLimit.hpp"

BWLimit::BWLimit ()
    : bandwidth (1024*1024),
      tsLast (0),
      bytesLast (0)
{
    
}
BWLimit::~BWLimit ()
{
}

void BWLimit::setBandwidth (uint32_t other)
{
    bandwidth = other;
}

uint64_t BWLimit::tsNext (uint32_t size)
{
    uint64_t ret = tsLast + (uint64_t)((1000000 * (bytesLast + size)) / bandwidth);
    bytesLast = size;
    tsLast = ret;
}

void BWLimit::setTsLast (uint64_t other)
{
    tsLast = other;
}
