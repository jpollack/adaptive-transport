#include "PTT.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "Timing.hpp"

PTT::PTT ()
    : m_base (nullptr),
      m_size (sysconf(_SC_PAGE_SIZE)),
      m_seq (1),
      m_head0 (0),
      m_head1 (0),
      m_tail (0)
{
    // for simplicities sake, lets just have pagesize number of
    // entries. memory is cheap. 
    size_t bytes = m_size * sizeof (Metadata); 

    // Alloc twice as much memory as we need
    if ((m_base = (uint8_t *) mmap (NULL, 2 * bytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED)
    {
	perror ("mmap");
	abort ();
    }

    // realloc back to what we really need.
    if ((m_base = (uint8_t *)mremap (m_base, 2 * bytes, bytes, 0)) == MAP_FAILED)
    {
	perror ("mremap");
	abort ();
    }

    // map the buffer's head after the tail :)
    if (mremap (m_base, 0, bytes, MREMAP_MAYMOVE | MREMAP_FIXED, m_base + bytes) == MAP_FAILED)
    {
	perror ("mremap second addr don't match");
	abort ();
    }	

    
    
    // const auto fd = fileno (tmpfile ());
    // if (ftruncate (fd, bytes) == -1)
    // {
    // 	perror ("ftruncate");
    // 	abort ();
    // }

    // map first half
    // if (mmap (m_base, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) != m_base)
    // {
    // 	perror ("mmap first addr don't match");
    // 	abort ();
    // }

    // // map second half
    // if (mmap (m_base + bytes, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) != (m_base + bytes))
    // {
    // 	perror ("mmap second addr don't match");
    // 	abort ();
    // }
}

PTT::~PTT ()
{
    size_t bytes = m_size * sizeof (Metadata); 
    munmap (m_base, bytes);
    munmap (m_base + bytes, bytes);
}

size_t PTT::advanceHead0 ()
{
    size_t ret = m_head0;
    m_head0 += 1;
    if (m_head0 == m_size)
    {
	m_head0 = 0;
    }
    return ret;
}

size_t PTT::advanceHead1 ()
{
    size_t ret = m_head1;
    m_head1 += 1;
    if (m_head1 == m_size)
    {
	m_head1 = 0;
    }
    return ret;
}

// return 0 on full.  returns sequence number.  advances head0.
SeqNum PTT::setSent (Timestamp tsSent)
{
    if (tsSent == 0)
    {
	tsSent = MicrosecondsSinceEpoch ();
    }

    Metadata *mbase = (Metadata *)m_base;

    mbase[this->advanceHead0 ()] = {.seq = m_seq, .tsSent = tsSent, .tsRecv = 0};
    
    // TODO: return 0 if full
    // return 0;
    
    return m_seq++;
}

// errors if sequence number isn't found.  Looks between head1(starT)
// and head0.  Advances head1.
bool PTT::setRecv (SeqNum seq, Timestamp tsRecv)
{
    if (tsRecv == 0)
    {
	tsRecv = MicrosecondsSinceEpoch ();
    }
    
    Metadata *mbase = (Metadata *)m_base;
    while (m_head1 != m_head0)
    {
	size_t ii = this->advanceHead1 ();
	
	if (mbase[ii].seq == seq)
	{
	    if (mbase[ii].tsRecv)
	    {
		// duplicate
		fprintf (stderr, "Error, ptt seq %lu dup tsRecv. [%lu -> %lu]\n", seq, mbase[ii].tsRecv, tsRecv);
		abort ();
	    }
	    mbase[ii].tsRecv = tsRecv;
	    return true;
	}
	
    }

    return false;
}

const PTT::Metadata * PTT::rptr () const
{
    if (readable ())
    {
	return ((const Metadata *) m_base) + m_tail;
    }
    else
    {
	return nullptr;
    }
}

void PTT::readAdvance (size_t num)
{
    m_tail += num;
    if (m_tail >= m_size)
    {
	m_tail -= m_size;
    }
}

size_t PTT::size () const
{
    return m_size;
}
SeqNum PTT::seq ()  const
{
    return m_seq;
}
// how many packets are in flight? -- head0 - head1.
size_t PTT::inFlight (void) const
{
    return indexDifference (m_head0, m_head1);
}

// how many entries are available for processing?  (number of packets
// received + number dropped).  head1 - tail

size_t PTT::readable (void) const
{
    return indexDifference (m_head1, m_tail);
}

size_t PTT::indexDifference (size_t minuend, size_t subtrahend) const
{
    if (minuend >= subtrahend)
    {
	return minuend - subtrahend;
    }
    return (minuend + m_size) - subtrahend;
}

