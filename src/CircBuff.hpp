 #pragma once

#include <atomic>
#include <assert.h>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <unistd.h>

#include <sys/mman.h>

constexpr uint64_t CACHE_LINE_SIZE = 64;

class CircBuff
{
public:
    CircBuff (int numPages = 1);
    ~CircBuff ();

    const uint8_t *rptr () const;
    void rptrAdvance (size_t num = 1);

    uint8_t *wptr ();
    void wptrAdvance (size_t num = 1);

    void clear ();

    size_t used () const;
    size_t capacity () const;
    size_t free () const;
    
private:
    size_t m_capacity;
    uint8_t *alignas (CACHE_LINE_SIZE) m_base;
    std::atomic<size_t> alignas (CACHE_LINE_SIZE) m_widx;
    std::atomic<size_t> alignas (CACHE_LINE_SIZE) m_ridx;
};
