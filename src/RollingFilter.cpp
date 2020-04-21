#include "RollingFilter.hpp"

RollingFilter::RollingFilter (const std::function<double(RollingWindow&, std::vector<double>&)>& func)
    : m_func (func),
      m_val (0),
      m_reg (4, 0.0)
{}

RollingFilter::~RollingFilter ()
{}

double RollingFilter::operator () (double val)
{
    m_wnd (val);
    m_val = m_func (m_wnd, m_reg);
    return m_val;
}

RollingFilter::operator double()
{
    return m_val;
}

int RollingFilter::size (void) const { return m_wnd.size (); }
void RollingFilter::size (int size)
{
    m_wnd.size (size);
}
    
