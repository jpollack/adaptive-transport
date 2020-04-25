#include "PacketQueue.hpp"
#include <new>
#include <cstring>

PacketQueue::PacketQueue ()
    : m_front (0),
      m_frontUsed (false),
      m_tx (0),
      m_unacked (0),
      m_size (8192),
      m_bytesQueued (0),
      m_buf (new Packet[m_size]) // FIXME, that's 11MB right there...
{}

PacketQueue::~PacketQueue ()
{
    // FIXME: wait until "empty"???
    delete[] m_buf;
    m_buf = nullptr;
}

Packet* PacketQueue::front ()
{
    bool expected = false;
    if (m_frontUsed.compare_exchange_strong (expected, true))
    {
	return m_buf + m_front;
    }
    return nullptr;
}

// Call after data is set -- this signals the semaphore
void PacketQueue::frontNext ()
{
    m_bytesQueued += m_buf[m_front].size ();
    m_front = (m_front == (m_size - 1)) ? 0 : (m_front + 1);
    m_frontUsed.store (false);
    m_frontSema.signal ();
    // FIXME: If top pushes past a tail it invalidates it.  So the tails
    // should be moved along too.
}
    
Packet* PacketQueue::tx (uint64_t timeout)
{
    // packets are queued from [back, top)
    while (m_tx == m_front)
    {
	if (!m_frontSema.wait (timeout))
	{
	    return nullptr;
	}
    }
    return m_buf + m_tx;
}

// Call after packet is sent.
void PacketQueue::txNext ()
{
    m_bytesQueued -= m_buf[m_tx].size ();
    m_tx = (m_tx == (m_size - 1)) ? 0 : (m_tx + 1);
    m_txSema.signal ();
}

// Retransmit the oldest sent packets which is missing tsRecv and
// tsAck. (done by copying data to ->top, then calling advanceTop()).
// returns false when no packets meet criteria. Modifies m_front _AND_
// m_unacked.
// Call from same thread as sent
bool PacketQueue::retransmitOlderThan (uint64_t ts)
{
    if ((m_unacked == m_tx)
	|| m_buf[m_unacked].tsRecv ()
	|| (m_buf[m_unacked].tsSent () > ts))
    {
	return false;
    }
    
// old enough, and it's not been acked.   get a pointer to the front to copy
// the packet.  FIXME: this copy is totally unnecessary...
    Packet *pkt;
    while ((pkt = this->front ()) == nullptr)	std::this_thread::yield ();
    pkt->seq (m_buf[m_unacked].seq ());
    pkt->isAck (false);
    pkt->size (m_buf[m_unacked].size ());
    memcpy (pkt->buf (), m_buf[m_unacked].buf (), pkt->size ());
    this->frontNext ();
    // advance unacked
    m_unacked = (m_unacked == (m_size - 1)) ? 0 : (m_unacked + 1);
	
    if ((m_unacked == m_tx)
	|| m_buf[m_unacked].tsRecv ()
	|| (m_buf[m_unacked].tsSent () > ts))
    {
	return false;
    }

    return true;

}

uint32_t PacketQueue::bytesQueued (void) const
{
    return m_bytesQueued;
}

// Call with sequence number, returns true if
// matching packet is found.  does NOT move cursor
Packet* PacketQueue::find (uint32_t seq)
{
    for (int ii = m_unacked; ii != m_tx; ii = (ii == (m_size - 1)) ? 0 : (ii + 1))
    {
	if (m_buf[ii].seq () == seq)
	{
	    return m_buf + ii;
	}
    }
    return nullptr;
}

// Call after setting tsRecv and tsAcked on packet.
// At this point, data payload could be freed.
void PacketQueue::unackedNext ()
{
    m_unacked = (m_unacked == (m_size - 1)) ? 0 : (m_unacked + 1);
}

Packet* PacketQueue::unacked ()
{
    if (m_unacked == m_tx)
    {
	return nullptr;
    }
    return m_buf + m_unacked;
}
