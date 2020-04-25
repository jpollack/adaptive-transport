#pragma once

#include "Packet.hpp"
#include <vector>
// Used for reliable sending of packets.
#include "concurrentqueue.h" // Included just for utility
#include "lightweightsemaphore.h"
#include <atomic>

class PacketQueue
{
public:
    PacketQueue ();
    ~PacketQueue ();


    Packet* front ();
    void frontNext (); // Call after data is set -- this signals the semaphore
    
    Packet* tx (uint64_t);
    void txNext ();  // Call after packet is sent.

    
    // Retransmit the oldest sent packets which is missing tsRecv and
    // tsAck. (done by copying data to ->top, then calling advanceTop()).
    // returns false when no packets meet criteria.
    bool retransmitOlderThan (uint64_t);

    uint32_t bytesQueued (void) const;

    // Call with sequence number, returns true if
    // matching packet is found, and moves ack cursor.
    Packet* find (uint32_t);

    Packet* unacked ();
    void unackedNext ();  // Call after setting tsRecv and tsAcked on packet.
			  // At this point, data payload could be freed.
    
private:
    using LightweightSemaphore = moodycamel::LightweightSemaphore;
    std::atomic<int> m_front;
    std::atomic<bool> m_frontUsed;
    std::atomic<int> m_tx;
    std::atomic<int> m_unacked;
    int m_size;
    std::atomic<uint32_t> m_bytesQueued;
    Packet *m_buf;
    LightweightSemaphore m_frontSema; // signaled when front moves
    LightweightSemaphore m_txSema; // signaled when front moves
};
