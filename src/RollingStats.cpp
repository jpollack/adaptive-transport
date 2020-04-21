#include "RollingStats.hpp"
#include <stdlib.h>
#include <cassert>
#include <cmath>

RollingStats::RollingStats ()
    : m_head (0),
      m_count (0),
      m_size (0),
      m_total (0),
      m_mean (0),
      m_variance (0)
{
}

RollingStats::~RollingStats ()
{
}

void RollingStats::operator () (double val)
{
    m_head = (m_head == (m_size - 1)) ? 0 : (m_head + 1);
    if (m_count++ >= m_size)
    {
	double oldval = m_buf[m_head];
	m_set.erase (m_set.find (oldval));
	m_total -= oldval;
    }
    int wsize = std::min (m_count, m_size);
    m_set.insert (val);
    m_total += val;
    m_mean = m_total / (double)wsize;
    m_buf[m_head] = val;

    // SLOW
    if (wsize > 1)
    {
	double m = 0;
	double s = 0;
	for (int k = 0; k < wsize; k++)
	{
	    double x = this->at (k);
	    double oldm = m;
	    m += (x - m) / (double)(k + 1);
	    s += (x - m) * (x - oldm);
	}
	m_variance = s / (double)(wsize - 1);
	if (val < m_min) { m_min = val; }
	if (val > m_max) { m_max = val; } 
    }
    else
    {
	m_min = val;
	m_max = val;
    }
}


const double& RollingStats::operator[] (int t) { return at (t); }

double& RollingStats::at (int t)
{
    int idx = m_head - t;
    while (idx < 0) { idx += m_size; }
    while (idx >= m_size) { idx -= m_size; }
    assert (idx >= 0 && idx < m_size);
    return m_buf[idx];
}

int RollingStats::size (void) const { return m_size; }

void RollingStats::size (int size)
{
    m_buf.resize (size);
    m_size = size;
    m_count = 0;
    m_head = 0;
    m_min = 0;
    m_max = 0;
    m_set.clear ();
    std::fill (m_buf.begin (), m_buf.end (), 0);
}

int RollingStats::count (void) const { return m_count; }
bool RollingStats::populated (void) const { return m_count >= m_size; }
double RollingStats::mean () const { return m_mean; }
double RollingStats::stdev () const { return sqrt (m_variance); }
double RollingStats::variance () const { return m_variance; }
double RollingStats::wmin () const { return *m_set.begin (); }
double RollingStats::wmax () const { return *m_set.rbegin (); }

double RollingStats::min () const { return m_min; }
double RollingStats::max () const { return m_max; }
