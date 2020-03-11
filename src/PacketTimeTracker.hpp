#pragma once
#include <vector>
#include <cstdint>

// This tracks the departure time of packets, so that we don't have to embed a
// timestamp.  General structure is like a ring buffer, because when we
// receive the time ack packet from the client, we update with the received tiems.


class PacketTimeTracker
{
public:
    PacketTimeTracker (size_t size);
    ~PacketTimeTracker ();

    // adds a new packet to tracking set, returns sequence number.  Call this
    // function _immediately_ before sending packet.  Advances head0.  return 0
    // on full
    uint32_t addPacket (uint64_t tsSent = 0);

    // returns false if sequence number isn't found.  Looks between head1
    // and head0.  Advances head1.
    void ackPacket (uint32_t seq, uint64_t tsRecv);

    // returns [seq, tsBase, tsRecv].  Advances tail.
    // this shuold really be changed to a more... expose the pointer approach
    std::tuple<uint32_t,uint64_t,uint64_t> getPacket ();

    // how many packets are in flight? -- head0 - head1.
    size_t inFlight (void) const;

    // how many entries are available for processing?  (number of packets
    // received + number dropped).  head1 - tail
    size_t available (void) const;

    // how many packets dropped
    // size_t dropped (void) const;

    // how many packets received
    // size_t received (void) const;

    // void reset (void);
private:
    struct Entry
    {
	uint32_t seq;
	uint64_t tsSent;
	uint64_t tsRecv;
    };
    std::vector<Entry> m_vec;
    uint32_t m_seq;
    size_t m_head0;  // where we add new packets (sent)
    size_t m_head1;  // where we add new packets (recv)
    size_t m_tail;

    size_t indexDifference (size_t minuend, size_t subtrahend) const;
};
