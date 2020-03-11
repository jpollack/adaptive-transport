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
    // TODO: should wait until all bytes queued are sent, and no retransmits
    // needed. this just does the former?
    while (m_bytesQueued && m_sendMap.size ())
    {
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
    
    this->enqueueSend ("ME");

    do
    {
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    } while  (m_bytesQueued);

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

void UDPStream::onTSReturnPacket (const std::string& payload)
{
    if (TSReturn::Valid (payload))
    {
	// magic matches.  so this is a tsreturn packet.
	for (const auto& tse : TSReturn::FromString (payload))
	{
	    if (!m_ptt.setRecv (tse.first, tse.second))
	    {
		fprintf (stderr, "Couldn't find matching packet\n");
		abort ();
	    }
	}
	
	return;
    }
    else
    {
	abort ();
    }
    
}

void UDPStream::onDataPacket (const std::string& payload)
{
    // just a normal data packet.
    uint32_t dseq = ((uint32_t *)payload.c_str ())[0];
    if (dseq == m_nextSeq)
    {
	// fast path
	memcpy (m_recvBuff.wptr (), 4+payload.c_str (), payload.size ()-4);
	m_recvBuff.wptrAdvance (payload.size () - 4);
	m_nextSeq++;
    }
    else
    {
	// out of order path
	m_recvMap[dseq] = payload.substr (4);
    }
    while (m_recvMap.find (m_nextSeq) != m_recvMap.end ())
    {
	const std::string& pl = m_recvMap[m_nextSeq];
	memcpy (m_recvBuff.wptr (), pl.c_str () + 4, pl.size () - 4);
	m_recvBuff.wptrAdvance (pl.size () - 4);
	m_recvMap.erase (m_nextSeq);
	m_nextSeq++;
    }
}

void UDPStream::onMetadataPacket (const std::string& payload)
{
    char type = payload.c_str ()[0];
    switch(type)
    {
    case 'E':
	// eof
	reof = true;
	break;
    default:
	fprintf (stderr, "unexpected metadata type '%c'(%d)\n", type, type);
	abort ();
    }
    
}

void UDPStream::receiverEntry (void)
{
    while (!m_done)
    {
	const auto& pkt = m_socket.recv ();
	const std::string& buf = std::get<2>(pkt);
	if (!buf.size ())
	{
	    continue;  // timeout
	}

	if (m_peer.size () == 0)
	{
	    m_peer = std::get<1>(pkt);
	}
	uint32_t seq = ((uint32_t *)buf.c_str ())[0];
	uint64_t ts = std::get<0>(pkt);
	char type = *(4 + buf.c_str ());
    
	// either way, we update our list of times
	m_tsRecv.emplace_back (seq, ts);
	switch (type)
	{
	case 'T':
	    this->onTSReturnPacket (buf.substr (5));
	    break;
	case 'D':
	    this->onDataPacket (buf.substr (5));
	    break;
	case 'M':
	    this->onMetadataPacket (buf.substr (5));
	    break;
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
	this->doTSReturn ();
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
}

void UDPStream::doTSReturn ()
{
    // TODO convert tsrecv into something on a circ buffer
    std::string payload = TSReturn::ToString (m_tsRecv);

    if (payload.size () > (mtu - 16)) // or it's been too long since the
					// last tsretrn
    {
	m_tsRecv.clear ();
	this->enqueueSend ("T" + payload);
    }
    
}
// bool UDPStream::send (const std::string& payload)
// {
//     // sending a zero length packet makes no sense
//     if (m_sendBuff.free () <= payload.size ())
//     {
// 	return false;
//     }

//     memcpy (m_sendBuff.wptr (), payload.c_str (), payload.size ());
//     m_sendBuff.wptrAdvance (payload.size ());
//     return true;
// }

// std::string UDPStream::recv (void)
// {
//     std::string payload;
//     m_recvQueue.wait_dequeue (payload);
//     return payload;
// }

