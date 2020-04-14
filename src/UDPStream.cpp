#include "UDPStream.hpp"
#include "Timing.hpp"
#include <algorithm>
// #include "TSReturn.hpp"

UDPStream::UDPStream ()
    : bandwidth (1024*1024),
      mtu (1400),
      rtt (1000000),
      m_done (false),
      m_nextSeq (1),
      m_recvBuff (1024),
      m_dseq (1),
      m_sender (&UDPStream::senderEntry, this),
      m_receiver (&UDPStream::receiverEntry, this),
      m_retransmit (&UDPStream::retransmitEntry, this),
      m_limiter (&UDPStream::limiterEntry, this),
      m_bytesQueued (0),
      m_tsUpdated (0)
{
    m_tsLast = 0;
    m_bytesLast = 0;
    m_socket.set_timestamps ();
    m_socket.set_reuseaddr ();
    m_socket.set_timeout (100000);
}

UDPStream::~UDPStream ()
{
    while (m_bytesQueued || m_rtxq.size ())
    {
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    m_done = true;
    m_receiver.join ();
    m_retransmit.join ();
    m_sender.join ();
}

const uint8_t *UDPStream::rptr ()
{
    return m_recvBuff.rptr ();
}

void UDPStream::rptrAdvance (size_t num)
{
    m_recvBuff.rptrAdvance (num);
}
uint32_t UDPStream::readable (void)
{
    return m_recvBuff.used ();
}
void UDPStream::setLocal (const Address& addr)
{
    m_socket.bind (addr);
}

void UDPStream::setRemote (const Address& addr)
{
    m_peer = addr;
}

uint64_t UDPStream::getDelay (uint32_t size)
{
    return (uint64_t)((1000000 * (m_bytesLast + size)) / bandwidth);
}

void UDPStream::enqueueSend (const std::string& payload, bool reliable)
{
    m_bytesQueued += payload.size ();
    m_sendQueue.enqueue ({.reliable = reliable, .bwlimited = true, .payload = payload});
}

void UDPStream::senderEntry (void)
{
    
    while (!m_done)
    {
	Payload pl;
	if (!m_sendQueue.wait_dequeue_timed (pl, std::chrono::milliseconds (100)))
	{
	    // fprintf (stderr, "sendQueue.deque timeout, retrying\n");
	    continue;
	}
	uint64_t tsNext = m_tsLast;
	if (pl.bwlimited)
	{
	    tsNext += this->getDelay (pl.payload.size ());
	}
	m_tsLast = wait_until (tsNext);
	uint32_t seq = m_ptt.setSent (m_tsLast);
	m_socket.sendto (m_peer, std::string ((const char*)&seq, 4) + pl.payload);

	m_bytesLast = pl.payload.size ();

	if (pl.reliable)
	{
	    m_rtxq.sentPacket (seq, m_tsLast, pl.payload);
	}

	m_bytesQueued -= pl.payload.size ();
    }
}

void UDPStream::send (const std::string& msg)
{
    uint32_t bytesRemaining = msg.size ();
    uint32_t idx = 0;
    while (bytesRemaining)
    {
	uint32_t cBytes = std::min (bytesRemaining, mtu);
	this->enqueueSend ("D" + std::string ((const char *)&m_dseq, 4) + msg.substr (idx, cBytes), true);
	m_dseq++;
	bytesRemaining -= cBytes;
	idx += cBytes;
    }
}

uint32_t UDPStream::bytesQueued (void) const
{
    return m_bytesQueued;
}

void UDPStream::onRecvData (uint32_t dseq, const char *base, uint32_t size)
{
    // if the data's too old (or a dup) just drop it entirely.
    if (dseq == m_nextSeq)
    {
	// fast path
	memcpy (m_recvBuff.wptr (), base, size);
	m_nextSeq++;
	m_recvBuff.wptrAdvance (size);
    }
    else
    {
	// out of order path
	if (dseq > m_nextSeq)
	{
	    m_recvMap[dseq] = std::string (base, base+size);
	}
    }
    while (m_recvMap.find (m_nextSeq) != m_recvMap.end ())
    {
	const std::string& pl = m_recvMap[m_nextSeq];
	memcpy (m_recvBuff.wptr (), pl.c_str (), pl.size ());
	m_recvBuff.wptrAdvance (pl.size ());
	m_recvMap.erase (m_nextSeq);
	m_nextSeq++;
    }
}

uint64_t UDPStream::onRecvAck (uint32_t seq, uint64_t tsRecv, uint64_t ackRecv)
{
    if (!m_ptt.setRecv (seq, tsRecv))
    {
	fprintf (stderr, "Couldn't find matching packet.  got ack for %u, %lu\n", seq, tsRecv);
	// abort ();
    }
    auto [tsSent, payload] = m_rtxq.dropPacket (seq);
    uint64_t rtt = ackRecv - tsSent;
    return rtt;
}


void UDPStream::receiverEntry (void)
{
    while (!m_done)
    {
	const auto [ts, raddr, buf] = m_socket.recv ();
	if (!buf.size ())
	{
	    continue;  // timeout
	}

	if (m_peer.size () == 0)
	{
	    m_peer = raddr;
	}

	const char *base = buf.c_str ();
	// TODO: replace this with a cast to a struct packetheader

	// get flags

	// if (flags & RequiresAck)
	// {
	//     // construct ack packet.
	//     std::string ackPacket = "A" + std::string (base + 1, 4) + std::string ((const char *)&ts, 8);
	//     m_socket.sendto (raddr)
	// }
	
	uint32_t seq = *((uint32_t *) base);
	char type = *(4 + base);

	if (type == 'A') // 0x41
	{
	    // uint32_t dseq = *((uint32_t *)(5 + base));
	    // TODO: this is so ugly, noone else thinks in bytes, fix this.
	    uint32_t aseq = *(uint32_t *)(base + 5);
	    uint64_t atsRecv = *(uint64_t *)(base + 5 + 4);
	    rtt = this->onRecvAck (aseq, atsRecv, ts);
	}
	else if (type == 'D')
	{
	    uint32_t dseq = *((uint32_t *)(5 + base));
	    const char *dbase = 9 + base;
	    uint32_t dsize = buf.size () - 9;
	    this->onRecvData (dseq, dbase, dsize);
	    // send Ack for the data
	    this->enqueueSend ("A"
			       + std::string ((const char *)&seq, 4)
			       + std::string ((const char*)&ts, 8), false);
	}
	else
	{
	    fprintf (stderr, "unexpected packet type '%c'(%d)\n", type, type);
	    abort ();
	}
	
    }
}

// everything older than now-2*rtt is retransmitted.
void UDPStream::retransmitEntry (void)
{
    while (!m_done)
    {
	// TODO: replace with rtt + 2 stdev
	uint64_t now = MicrosecondsSinceEpoch ();

	for (const auto& seq : m_rtxq.olderThan (now - (2*rtt)))
	{
	    auto [ tsSent, payload ] = m_rtxq.dropPacket (seq);
	    this->enqueueSend (payload, true);
	}
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
}

void UDPStream::limiterStep ()
{
    // tsUpdated -- timestamp when we last made a change to the send
    // rate. Ignore all data from packets sent before this time.

	/* packet stats:
	   tsNow
	   droppedRatio [0,1]
	   yv = tsDiff = tsRecv - tsSent
	   xv = tsSent - tsSent(1)
	   do linear regression to get slope and intercept
	   if slope > threshold, then slow down
	   
	   

	 */
    // read all available entries from m_ptt into a local vector

    int nrecv = 0;
    int ndropped = 0;

    std::vector<uint32_t> yv;
    std::vector<uint32_t> xv;

    uint64_t x0 = 0;
    while (m_ptt.readable ())
    {
	const PTT::Metadata *mp = m_ptt.rptr ();
	if (onPacketMetadata)
	{
	    onPacketMetadata (mp->seq, mp->tsSent, mp->tsRecv);
	}
	
	if (m_tsUpdated > mp->tsSent)
	{
	    continue;
	}

	if (mp->tsRecv)
	{
	    // Do something with the time diff
	    if (!nrecv)
	    {
		x0 = mp->tsSent;
	    }
	    nrecv++;
	    yv.emplace_back (mp->tsRecv - mp->tsSent);
	    xv.emplace_back (mp->tsSent - x0);
	}
	else
	{
	    ndropped++;
	}
	m_ptt.readAdvance (1);
    }

    printf ("(%d,%d)\t", nrecv, ndropped);
    for (auto y : yv)
    {
	printf ("%d ", y);
    }
    printf ("\n");
    
    
}

void UDPStream::limiterEntry (void)
{
    int minPackets = 8;

    while (!m_done)
    {
	if (m_ptt.readable () >= minPackets)
	{
	    this->limiterStep ();
	}
	std::this_thread::sleep_for (std::chrono::milliseconds (2));
    }
}
