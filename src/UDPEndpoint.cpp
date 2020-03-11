#include "UDPEndpoint.hpp"
#include "Timing.hpp"
#include "TSReturn.hpp"


UDPEndpoint::UDPEndpoint (PTT& ptt)
    : m_done (false),
      m_ptt (ptt),
      m_receiver (&UDPEndpoint::receiverEntry, this)

{
    m_socket.set_timestamps ();
    m_socket.set_reuseaddr ();
    m_socket.set_timeout (100000);
}

UDPEndpoint::~UDPEndpoint ()
{
    m_done = true;
    // m_sender.join ();
    m_receiver.join ();
}

void UDPEndpoint::setLocal (const Address& addr)
{
    m_socket.bind (addr);
}

void UDPEndpoint::setPeer (const Address& addr)
{
    m_peer = addr;
}

uint64_t UDPEndpoint::sendAt (const std::string& payload, uint64_t when)
{
    uint64_t now = wait_until (when);
    uint32_t seq = m_ptt.setSent (now);
    if (!seq)
    {
	fprintf (stderr, "packet queue full\n");
	exit (1);
    }
    m_socket.sendto (m_peer, std::string ((const char*)&seq, 4) + payload);

    // TODO: process the packettimetracker info?  which leads to updating bandwidth

    return now;
}

std::pair<uint32_t, std::string> UDPEndpoint::queueRecv (void)
{
    std::pair<uint32_t,std::string> el;
    m_recvQueue.wait_dequeue (el);
    return el;
}

std::pair<uint32_t,std::string> UDPEndpoint::recv (void)
{
    const auto& pkt = m_socket.recv ();
    // Receive a packet.  Split out the sequence number.  If it's a timestamp
    // return packet, then update the packet time tracker.  if it's a data
    // packet, then update our scratch buffer for returning.

    const std::string& buf = std::get<2>(pkt);
    if (!buf.size ())
    {
	return std::make_pair (0,"");
    }

    uint32_t seq = *((uint32_t *)buf.c_str ());

    uint64_t ts = std::get<0>(pkt);
    if (m_peer.size () == 0)
    {
	m_peer = std::get<1>(pkt);
    }
    // either way, we update our list of times
    m_tsRecv.emplace_back (seq, ts);

    // add 4 for sequence number, 16 for header size, and 8 per entry
    if ( (4 + 16 + ((m_tsRecv.size () + 1) * 8)) > 1500)
    {
	// tsRecv is 'full', so serialize it and send immediately
	this->sendAt (TSReturn::ToString (m_tsRecv), 0);
	m_tsRecv.clear ();
    }
    std::string payload = buf.substr (4);
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
	return std::make_pair (0,"");
    }
    else
    {
	// just a normal data packet.
	return std::make_pair (seq, payload);
    }
    
}


// void UDPEndpoint::senderEntry (void)
// {
//     while (!m_done)
//     {
// 	TimedPacket pkt;
// 	if (!m_sendQueue.wait_dequeue_timed (pkt, std::chrono::milliseconds (100)))
// 	{
// 	    // timeout
// 	    continue;
// 	}
// 	// send packet
// 	uint64_t when = sendAt (pkt.timestamp, pkt.addr, pkt.payload);
// 	pkt.timestampProm.set_value (when);
//     }

// }

void UDPEndpoint::receiverEntry (void)
{
    while (!m_done)
    {
	const auto el = this->recv ();
	if (el.first)
	{
	    m_recvQueue.enqueue (std::move (el));
	}
    }

}
