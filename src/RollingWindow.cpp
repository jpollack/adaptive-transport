#include "RollingWindow.hpp"
#include <stdlib.h>
#include <cassert>

RollingWindow::RollingWindow ()
    : m_head (0),
      m_count (0),
      m_size (0)
{}

RollingWindow::~RollingWindow ()
{}

void RollingWindow::operator () (double val)
{
    m_count++;
    m_head = (m_head == (m_size - 1)) ? 0 : (m_head + 1);
    m_buf[m_head] = val;
}

const double& RollingWindow::operator[] (int t)
{
    int idx = m_head - t;
    while (idx < 0) { idx += m_size; }
    while (idx >= m_size) { idx -= m_size; }
    assert (idx >= 0 && idx < m_size);
    return m_buf[idx];
}

int RollingWindow::size (void) const { return m_size; }
void RollingWindow::size (int size)
{
    m_buf.resize (size);
    m_size = size;
    m_count = 0;
    m_head = 0;
    std::fill (m_buf.begin (), m_buf.end (), 0);
}

int RollingWindow::count (void) const { return m_count; }
