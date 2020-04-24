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
      updateBandwidth (true),
      m_done (false),
      m_nextSeq (1),
      m_recvBuff (1024),
      m_dseq (1),
      m_sender (&UDPStream::senderEntry, this),
      m_receiver (&UDPStream::receiverEntry, this),
      m_limiter (&UDPStream::limiterEntry, this),
      m_tsUpdated (0)
{
    rtt.size (16);
}

UDPStream::~UDPStream ()
{
    while (this->bytesQueued ())
    {
	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

    m_done = true;
    m_receiver.join ();
    m_sender.join ();
    m_limiter.join ();
}

const uint8_t *UDPStream::rptr () { return m_recvBuff.rptr (); }
void UDPStream::rptrAdvance (size_t num) { m_recvBuff.rptrAdvance (num); }
uint32_t UDPStream::readable (void) { return m_recvBuff.used (); }
void UDPStream::setLocal (const Address& addr) { m_socket.bind (addr); }
void UDPStream::setRemote (const Address& addr) { m_peer = addr; }

void UDPStream::senderEntry (void)
{
    TokenBucket tb;
    while (!m_done)
    {
	Packet *pkt = m_packetQueue.tx (10000);
	if (pkt == nullptr)
	{
	    continue; // timeout
	}
	while (!tb.consume (pkt->size (), bandwidth, 128*1024))
	{
	    // std::this_thread::yield ();
	    std::this_thread::sleep_for (std::chrono::microseconds (100));
	}
	pkt->tsSent (MicrosecondsSinceEpoch ());
	m_socket.sendto (m_peer, pkt->buf (), pkt->size ());
	m_packetQueue.txNext ();
    }
}

void UDPStream::send (const std::string& msg)
{
    uint32_t bytesRemaining = msg.size ();
    uint32_t idx = 0;
    Packet *pkt = nullptr;
    while (bytesRemaining)
    {
	while ((pkt = m_packetQueue.front ()) == nullptr)
	{
	    std::this_thread::yield ();
	}
	pkt->seq (m_dseq++);
	pkt->isAck (false);
	pkt->size (4 + std::min (bytesRemaining, Packet::MTU));
	msg.copy (pkt->dataPayload (), pkt->size () - 4, msg.size () - bytesRemaining);
	bytesRemaining -= (pkt->size () - 4);
	m_packetQueue.frontNext ();
    }
}

uint32_t UDPStream::bytesQueued (void) const
{
    return m_packetQueue.bytesQueued ();
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

void UDPStream::onMetadata (uint32_t seq, uint64_t tsSent, uint64_t tsRecv, uint64_t tsAck)
{
    if (tsAck)
    {
	rtt (tsAck - tsSent);
    }
    else
    {
	// dropped
    }
}

void UDPStream::receiverEntry (void)
{
    Packet pkt;
    memset (&pkt, 0, sizeof(Packet));
    while (!m_done)
    {
	pkt.size (0);
	const auto raddr = m_socket.recv (pkt);

	if (pkt.size ()) // if we received a packet (and not a timeout)
	{
	    if (pkt.size () < 4)
	    {
		fprintf (stderr, "Unexpected packet of length %d\n", pkt.size ());
		abort ();
	    }

	    m_peer = raddr;

	    if (pkt.isAck ())
	    {
		Packet *opkt = m_packetQueue.find (pkt.seq ());
		if (opkt)
		{
		    opkt->tsRecv (pkt.tsRecvAck ());
		    opkt->tsAck (pkt.tsRecv ());
		    // rtt (opkt->tsAck () - opkt->tsSent ());
		    // // fprintf (stderr, "RTT: %lf\t%lf\n", rtt.mean (), rtt.stdev ());
		    // while (m_packetQueue.unacked ()
		    // 	   && m_packetQueue.unacked ()->tsRecv ()
		    // 	   && m_packetQueue.unacked ()->tsAck ())
		    // {
		    // 	m_packetQueue.unackedNext ();
		    // }
		}
	    }
	    else
	    {
		// Data packet
		this->onRecvData (pkt.seq (), pkt.dataPayload (), pkt.size () - 4);
		// Reuse the packet to create the ack.
		pkt.isAck (true);
		pkt.size (12);
		pkt.tsRecvAck (pkt.tsRecv ());
		// Send ack immediately
		m_socket.sendto (raddr, pkt.buf (), pkt.size ());
	    }

	    // this->retransmit ();
	}
	uint64_t now = MicrosecondsSinceEpoch ();
	uint64_t timeout = std::max (rtt.populated () ? rtt.mean () * 2 : 1000000.0, 10000.0);
	uint64_t dropTime = now - timeout;
	// starting at the oldest packet.. 
	// * if tsRecv and tsAck are set, then it's been acked.
	// * while it's older than now - timeout or is an acked packet,
	// advance
	
	Packet *upkt = m_packetQueue.unacked ();
	while (upkt 
	       && (upkt->tsRecv () // it's been acked 
		   || (upkt->tsSent () < dropTime))) // it's too old
	{
	    if (!upkt->tsRecv ())
	    {
		// retransmit
		Packet *fpkt;
		while ((fpkt = m_packetQueue.front ()) == nullptr) 	    std::this_thread::yield ();
		memcpy (fpkt->buf (), upkt->buf (), upkt->size ());
		fpkt->size (upkt->size ());
		m_packetQueue.frontNext ();
	    }

	    // Do something with the metadata
	    this->onMetadata (upkt->seq (), upkt->tsSent (), upkt->tsRecv (), upkt->tsAck ());

	    m_packetQueue.unackedNext ();
	    upkt = m_packetQueue.unacked ();
	}

    }

}

int UDPStream::retransmit (void) // returns number dropped
{
    // TODO: replace with rtt + 2 stdev
    uint64_t now = MicrosecondsSinceEpoch ();
    uint64_t timeout = std::max (rtt.populated () ? rtt.mean () * 2 : 1000000.0, 10000.0);

    int ndropped = 0;
    while (m_packetQueue.retransmitOlderThan (now - timeout))
    {
	ndropped++;
    }
    return ndropped;
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

		printf ("%lu\t(%d,%d)\t%d\t%d\t%f\t%d\t%f\n", this->bandwidth,nrecv, ndropped, symin0, cs, sc, state, x);

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
			
			double newBandwidth = (double) this->bandwidth + ((double)this->mtu / ((double)abs (symin0) / 1000000.0));
			if (newBandwidth < static_cast<double>(std::numeric_limits<uint32_t>::max () / 2))
			{
			    this->bandwidth = (uint32_t)newBandwidth;
			}
			else
			{
			    this->bandwidth = (std::numeric_limits<uint32_t>::max () / 2);
			}
		    }

		}
	    }
	    
	}
	std::this_thread::sleep_for (std::chrono::milliseconds (5));
    }
}
