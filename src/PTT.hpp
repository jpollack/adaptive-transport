#pragma once

#include <cstdint>
#include <cstddef>

using SeqNum = uint32_t;
using Timestamp = uint64_t;

// This is for tracking the time packets are sent and received in a ring
// buffer.  A separeate modulecan do the data processing on them.

class PTT
{
public:
    struct Metadata
    {
	SeqNum seq;
	Timestamp tsSent;
	Timestamp tsRecv;
    };
    
    PTT ();
    ~PTT ();

    /* Adds a new entry for a packet to the tracking set, assigning it and
     * returning a unique sequence number.  Intent is to call this
     * immediately before sending the packet */
    SeqNum setSent (Timestamp tsSent = 0);
    size_t inFlight () const;

    // must be sorted by sequence number
    bool setRecv (SeqNum seq, Timestamp tsRecv = 0);
    // // Maybe add option for parsing the whole packet  

    const Metadata *rptr () const;
    void readAdvance (size_t num = 1);  
    size_t readable () const;
    size_t size () const;
    SeqNum seq () const;
    
private:
    size_t indexDifference (size_t minuend, size_t subtrahend) const;
    size_t advanceHead0 ();
    size_t advanceHead1 ();
    uint8_t *m_base;
    size_t m_size;
    SeqNum m_seq;
    size_t m_head0;
    size_t m_head1;
    size_t m_tail;
};
