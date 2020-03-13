#include "UDPStream.hpp"
#include "Timing.hpp"
#include <algorithm>
#include "TSReturn.hpp"

UDPStream::UDPStream ()
    : reof (false),
      bandwidth (1024*1024),
      mtu (1400),
      m_done (false),
      m_nextSeq (1),
      m_recvBuff (1024),
      m_dseq (1),
      m_sender (&UDPStream::senderEntry, this),
      m_receiver (&UDPStream::receiverEntry, this),
      m_controller (&UDPStream::controllerEntry, this),
      m_bytesQueued (0)
{
    m_tsLast = 0;
    m_bytesLast = 0;
    m_socket.set_timestamps ();
    m_socket.set_reuseaddr ();
    m_socket.set_timeout (100000);
}

UDPStream::~UDPStream ()
{
    while (m_bytesQueued && m_sendMap.size ())
    {
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    m_done = true;
    m_receiver.join ();
    m_controller.join ();
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

void UDPStream::enqueueSend (const std::string& packet)
{
    m_bytesQueued += packet.size ();
    m_sendQueue.enqueue (packet);
}

void UDPStream::senderEntry (void)
{
    
    while (!m_done)
    {
	std::string payload;
	if (!m_sendQueue.wait_dequeue_timed (payload, std::chrono::milliseconds (100)))
	{
	    // fprintf (stderr, "sendQueue.deque timeout, retrying\n");
	    continue;
	}
	
	uint64_t tsNext = m_tsLast + this->getDelay (payload.size ());
	m_tsLast = wait_until (tsNext);
	uint32_t seq = m_ptt.setSent (m_tsLast);
	m_socket.sendto (m_peer, std::string ((const char*)&seq, 4) + payload);
	m_bytesLast = payload.size ();
	m_sendMap[seq] = payload;
	m_bytesQueued -= payload.size ();
    }
}

void UDPStream::send (const std::string& msg)
{
    uint32_t bytesRemaining = msg.size ();
    uint32_t idx = 0;
    while (bytesRemaining)
    {
	uint32_t cBytes = std::min (bytesRemaining, mtu);
	this->enqueueSend ("D" + std::string ((const char *)&m_dseq, 4) + msg.substr (idx, cBytes));
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

void UDPStream::onRecvAck (uint32_t seq, uint64_t tsRecv)
{
    if (!m_ptt.setRecv (seq, tsRecv))
    {
	fprintf (stderr, "Couldn't find matching packet.  got ack for %u, %lu\n", seq, tsRecv);
	// abort ();
    }
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
	uint32_t seq = *((uint32_t *) base);
	char type = *(4 + base);

	switch (type)
	{
	case 'A':
	{
	    uint32_t asize = (buf.size () - 5) / (12); // 8(timestamp) +
						       // 4(seq)
	    for (int ii=0; ii < asize; ++ii)
	    {
		// TODO: this is so ugly, noone else thinks in bytes, fix this.
		uint32_t aseq = *(uint32_t *)(base + 5 + (ii * 12));
		uint64_t atsRecv = *(uint64_t *)(base + 5 + (ii * 12) + 4);
		this->onRecvAck (aseq, atsRecv);
	    }
	    break;
	}
	case 'D':
	{
	    uint32_t dseq = *((uint32_t *)(5 + base));
	    const char *dbase = 9 + base;
	    uint32_t dsize = buf.size () - 9;
	    this->onRecvData (dseq, dbase, dsize);
	    // send Ack for the data
	    this->enqueueSend ("A" + std::string ((const char *)&seq, 4) + std::string ((const char*)&ts, 8));
	    break;
	}
	default:
	    fprintf (stderr, "unexpected packet type '%c'(%d)\n", type, type);
	    abort ();
	    break;
	}
	
    }
}
void UDPStream::controllerEntry (void)
{
    while (!m_done)
    {
	while (m_ptt.readable ())
	{
	    const PTT::Metadata *mp = m_ptt.rptr ();
	    if (mp->tsRecv)
	    {
		m_sendMap.erase (mp->seq);
		// Do something with the time diff
		if (onAckedFunc)
		{
		    onAckedFunc (mp->seq, mp->tsSent, mp->tsRecv);
		}
	    }
	    else
	    {
		// it's dropped, so retransmit
		const auto payload = m_sendMap[mp->seq];
		m_sendMap.erase (mp->seq);
		this->enqueueSend (payload);
		if (onDroppedFunc)
		{
		    onDroppedFunc (mp->seq);
		}
	    }
	    m_ptt.readAdvance (1);
	}
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
}

