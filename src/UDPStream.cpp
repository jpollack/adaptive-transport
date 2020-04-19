#include "UDPStream.hpp"
#include "Timing.hpp"
#include <algorithm>
#include <queue>
#include <random>
#include "TokenBucket.hpp"
// #include "TSReturn.hpp"

UDPStream::UDPStream ()
    : bandwidth (1024*1024),
      mtu (1400),
      rtt (1000000), // initilize to 1 second.
      updateBandwidth (true),
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
    m_limiter.join ();
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

void UDPStream::enqueueSend (const std::string& payload, bool reliable)
{
    m_bytesQueued += payload.size ();
    m_sendQueue.enqueue ({.reliable = reliable, .bwlimited = true, .payload = payload});
}

void UDPStream::senderEntry (void)
{
    TokenBucket tb;
    
    while (!m_done)
    {
	Payload pl;
	if (!m_sendQueue.wait_dequeue_timed (pl, std::chrono::milliseconds (100)))
	{
	    continue; // timeout
	}

	if (pl.bwlimited)
	{
	    while (!tb.consume (pl.payload.size (), bandwidth, 128*1024))
	    {
		std::this_thread::yield ();		// not sendable right now
	    }
	    // sendable
	}
	uint64_t tsSent = MicrosecondsSinceEpoch ();
	uint32_t seq = m_ptt.setSent (tsSent);
	m_socket.sendto (m_peer, std::string ((const char*)&seq, 4) + pl.payload);

	if (pl.reliable)
	{
	    m_rtxq.sentPacket (seq, tsSent, pl.payload);
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

	// if (m_peer.size () == 0)
	// {
	    m_peer = raddr;
	// }

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

std::tuple<int,int,int,int> UDPStream::limiterStats ()
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

    std::vector<int> yv;
    std::vector<int> xv;

    int ymin;
    int ymax;
    
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
	    m_ptt.readAdvance (1);
	    continue;
	}

	if (mp->tsRecv)
	{
	    // Do something with the time diff
	    if (!nrecv)
	    {
		x0 = mp->tsSent;
		ymin = mp->tsRecv - mp->tsSent;
		ymax = mp->tsRecv - mp->tsSent;
	    }
	    yv.emplace_back (mp->tsRecv - mp->tsSent);
	    xv.emplace_back (mp->tsSent - x0);
	    if (yv.back () < ymin)
	    {
		ymin = yv.back ();
	    }
	    if (yv.back () > ymax)
	    {
		ymax = yv.back ();
	    }
	    nrecv++;
	    
	}
	else
	{
	    ndropped++;
	}
	m_ptt.readAdvance (1);
    }

    return std::make_tuple (ymin,ymax,nrecv,ndropped);
    
}

void UDPStream::limiterEntry (void)
{
    int minPackets = 12;
    int wnd = 4;
    
    int iter = 0;
    std::queue<int> q;
    int y0 = 0;
    int symin0 = 0;
    int symin1 = 0;
    int gymin = 0;
    int cs = 0;
    double sc = 0;
    int state = 0;
    
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(0.0,1.0);
    while (!m_done)
    {
	if (m_ptt.readable () >= minPackets)
	{
	    auto [ymin, ymax, nrecv, ndropped] = this->limiterStats ();
	    if ((nrecv + ndropped) == 0)
	    {
		continue;
	    }
	    iter++;
	    q.push (ymin);
	    y0 = y0 + ymin;
	    if (q.size () < wnd)
	    {
		continue;
	    }
	    while (q.size () > wnd)
	    {
		y0 = y0 - q.front ();
		q.pop ();
	    }
	    symin1 = symin0;
	    symin0 = y0 / wnd;
	    if (symin1)
	    {

		gymin = symin0 - symin1;
		cs = cs + gymin;
		sc = (double)cs /(double)symin1;
		state = 0;
		if (sc > 0.20)
		{
		    state = -1;
		}
		if (ndropped > 1)
		{
		    state = -1;
		}
		if (sc < -0.20)
		{
		    cs = 0;
		    m_tsUpdated = MicrosecondsSinceEpoch ();
		}

		double x = distribution (generator);
		if ((state == 0) && (x<0.25))
		{
		    state = 1;
		}

		printf ("%d\t(%d,%d)\t%d\t%d\t%f\t%d\t%f\n", this->bandwidth,nrecv, ndropped, symin0, cs, sc, state, x);

		if (state && updateBandwidth)
		{
		    if (state < 0)
		    {
			this->bandwidth = (double)this->bandwidth * 0.75;
			cs = 0;
			m_tsUpdated = MicrosecondsSinceEpoch ();
		    
		    }

		    if (state > 0)
		    {
			this->bandwidth += ((double)this->mtu / ((double)symin0 / 1000000.0));
		    }

		}
	    }
	    
	}
	std::this_thread::sleep_for (std::chrono::milliseconds (5));
    }
}
