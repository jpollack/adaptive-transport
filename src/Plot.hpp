#pragma once

#include "readerwriterqueue.h"
#include <thread>
#include <vector>
#include <atomic>
class Plot
{
public:
    Plot ();
    ~Plot ();
    void plot (const std::vector<double>& xv, const std::vector<double>&yv);
    
private:
    using PlotPayload = std::tuple<std::vector<double>,std::vector<double>>;
    using PlotQueue = moodycamel::BlockingReaderWriterQueue<PlotPayload>;
    void plotEntry (void);
    std::atomic<int> m_numQueued;
    PlotQueue m_queue;
    bool m_done;
    std::thread m_thread;
    
};
