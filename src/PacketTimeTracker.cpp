#include "PacketTimeTracker.hpp"
#include "Timing.hpp"

PacketTimeTracker::PacketTimeTracker (size_t size)
    : m_vec (size),
      m_seq (1),
      m_head0 (0),
      m_head1 (0),
      m_tail (0)
{}

PacketTimeTracker::~PacketTimeTracker ()
{}

// return 0 on full.  returns sequence number.  advances head0.
uint32_t PacketTimeTracker::addPacket (uint64_t tsSent)
{
    if (tsSent == 0)
    {
	tsSent = MicrosecondsSinceEpoch ();
    }

    // TODO: return 0 if full
    // return 0;
    
    // add entry
    m_vec[m_head0].seq = m_seq;
    m_vec[m_head0].tsSent = tsSent;
    m_vec[m_head0].tsRecv = 0;

    m_seq += 1;
    
    // advance head0
    m_head0 += 1;
    if (m_head0 == m_vec.size ())
    {
	m_head0 = 0;
    }

    return m_seq - 1;
}

// errors if sequence number isn't found.  Looks between head1(starT)
// and head0.  Advances head1.
void PacketTimeTracker::ackPacket (uint32_t seq, uint64_t tsRecv)
{
    while (m_head1 != m_head0)
    {
	size_t ii = m_head1;
	
	m_head1 += 1;
	if (m_head1 == m_vec.size ())
	{
	    m_head1 = 0;
	}

	if (m_vec[ii].seq == seq)
	{
	    if (m_vec[ii].tsRecv)
	    {
		// duplicate
		fprintf (stderr, "Error, ptt seq %lu dup tsRecv. [%lu -> %lu]\n", seq, m_vec[ii].tsRecv, tsRecv);
		for (const auto& el : m_vec)
		{
		    printf ("%d\t%lu\t%lu\n", el.seq, el.tsSent, el.tsRecv);
		}
		exit (1);
	    }
	    m_vec[ii].tsRecv = tsRecv;
	    return;
	}
	
    }
    
    fprintf (stderr, "Error, ptt seq %lu not found.\n", seq);
    exit (1);
}

// returns [seq, tsBase, tsRecv].  Advances tail.

// TODO: what does it mean to call this before any packets have been acked?
// perhaps we should just return a subset which can be empty
std::tuple<uint32_t,uint64_t,uint64_t> PacketTimeTracker::getPacket ()
{
    size_t ii = m_tail;
    m_tail += 1;
    if (m_tail == m_vec.size ())
    {
	m_tail = 0;
    }
    return std::make_tuple (m_vec[ii].seq, m_vec[ii].tsSent, m_vec[ii].tsRecv);
}

// how many packets are in flight? -- head0 - head1.
size_t PacketTimeTracker::inFlight (void) const
{
    return indexDifference (m_head0, m_head1);
}

// how many entries are available for processing?  (number of packets
// received + number dropped).  head1 - tail

size_t PacketTimeTracker::available (void) const
{
    return indexDifference (m_head1, m_tail);
}

size_t PacketTimeTracker::indexDifference (size_t minuend, size_t subtrahend) const
{
    if (minuend >= subtrahend)
    {
	return minuend - subtrahend;
    }
    return (minuend + m_vec.size ()) - subtrahend;
}
