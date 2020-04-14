#pragma once

#include <unordered_map>
#include <vector>
#include <mutex>

// This crude structure holds data payloads, sequence numbers and timestamp
// sent for retransmit purposes.  Can be re-architected into a circ buffer
// with fixed packet sizes (set to MTU)

class RTXQueue
{
public:
    RTXQueue ();
    ~RTXQueue ();

    // adds the packet and timestamp to the queue TODO: make return sequence number
    void sentPacket (uint32_t seq, uint64_t tsSent, const std::string& payload);
    std::pair<uint64_t, std::string> dropPacket (uint32_t seq);
    std::vector<uint32_t> olderThan (uint64_t ts);
    std::vector<uint32_t> unacked (uint64_t ts);
    size_t size () const;

private:
    std::unordered_map<uint32_t,std::pair<uint64_t,std::string> > m_map;
    std::mutex m_mutex;
};
