#pragma once

// Encapsulates putting the packets back in order and exposing a generic fd
// like interface
#include "CircBuff.hpp"
#include <unordered_map>

class ReadStream
{
public:
    ReadStream ();
    ~ReadStream ();

    void addPacket (uint32_t seq, const std::string& payload);
    const uint8_t *rptr () const;
    void rptrAdvance (size_t num = 1);
    size_t available () const; // same as 'readable'
    
private:
    uint32_t m_nextSeq;
    std::unordered_map<uint32_t,std::string> m_map;
    CircBuff m_cb;
};
