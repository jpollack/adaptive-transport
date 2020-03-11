#include "CircBuff.hpp"

CircBuff::CircBuff (int numPages)
    : m_capacity (sysconf (_SC_PAGE_SIZE) * numPages),
      m_base (nullptr),
      m_widx (0),
      m_ridx (0)
{
    if ((m_base = (uint8_t *) mmap (NULL, 2 * m_capacity, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0)) == MAP_FAILED)
    {
	perror ("mmap");
	abort ();
    }

    // realloc back to what we really need.
    if ((m_base = (uint8_t *)mremap (m_base, 2 * m_capacity, m_capacity, 0)) == MAP_FAILED)
    {
	perror ("mremap");
	abort ();
    }

    // map the buffer's head after the tail :)
    if (mremap (m_base, 0, m_capacity, MREMAP_MAYMOVE | MREMAP_FIXED, m_base + m_capacity) == MAP_FAILED)
    {
	perror ("mremap second addr don't match");
	abort ();
    }	
    
}

CircBuff::~CircBuff ()
{
    munmap (m_base, m_capacity);
    munmap (m_base + m_capacity, m_capacity);
}

const uint8_t *CircBuff::rptr () const
{
    size_t ridx = m_ridx.load ();
    return m_base + ridx;
}
void CircBuff::rptrAdvance (size_t num)
{
    bool success = false;
    size_t ridx;
    size_t rnext;
    do
    {
	ridx = m_ridx.load (std::memory_order_acquire);
	rnext = ridx + num;
	if (rnext >= m_capacity)
	{
	    rnext -= m_capacity;
	}
	success = m_ridx.compare_exchange_weak (ridx, rnext, std::memory_order_release);
    } while (!success);
}

uint8_t *CircBuff::wptr ()
{ 
    size_t widx = m_widx.load ();
    return m_base + widx;
}
void CircBuff::wptrAdvance (size_t num)
{
    bool success = false;
    size_t widx;
    size_t wnext;
    do
    {
	widx = m_widx.load (std::memory_order_acquire);
	wnext = widx + num;
	if (wnext >= m_capacity)
	{
	    wnext -= m_capacity;
	} 
	success = m_widx.compare_exchange_weak (widx, wnext, std::memory_order_release);
    } while (!success);
}

void CircBuff::clear ()
{
    m_ridx.store (0);
    m_widx.store (0);
}

size_t CircBuff::used () const
{
    const size_t widx = m_widx.load ();
    const size_t ridx = m_ridx.load ();
    return ((widx + m_capacity) - ridx) & (m_capacity - 1);
}
size_t CircBuff::capacity () const
{
    return m_capacity;
}
size_t CircBuff::free () const
{
    return m_capacity - used ();
}
