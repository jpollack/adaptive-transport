#include "Plot.hpp"
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

Plot::Plot (void)
    : m_done (false),
      m_thread (&Plot::plotEntry, this)
{}

Plot::~Plot (void)
{
    // while (m_numQueued)
    // {
    // 	std::this_thread::sleep_for (std::chrono::milliseconds (10));
    // }

    m_done = true;
    m_thread.join ();
}

void Plot::plot (const std::vector<double>& xv, const std::vector<double>&yv, const std::vector<double>& zv)
{
    m_numQueued++;
    m_queue.enqueue ({xv,yv, zv});
}

void Plot::plotEntry (void)
{
    // plt::Plot ipo;
    plt::figure ();
    while (!m_done)
    {
	std::tuple<std::vector<double>,std::vector<double>,std::vector<double>> el;
	if (!m_queue.wait_dequeue_timed (el, std::chrono::milliseconds (100)))
	{
	    continue;
	}
	plt::clf ();
	plt::plot (std::get<0>(el), std::get<1>(el));
	plt::plot (std::get<0>(el), std::get<2>(el));
	plt::ylim (-10000, 10000);
	// ipo.update (std::get<0>(el), std::get<1>(el));
	plt::pause (0.001);
	m_numQueued--;
    }
}
