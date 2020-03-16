#include "RTXQueue.hpp"

RTXQueue::RTXQueue ()
{
    
}
RTXQueue::~RTXQueue ()
{
    
}

    // adds the packet and timestamp to the queue TODO: make return sequence number
void RTXQueue::sentPacket (uint32_t seq, uint64_t tsSent, const std::string& payload)
{
    std::lock_guard<std::mutex> guard (m_mutex);
    m_map[seq] = std::make_pair (tsSent, payload);
}

std::pair<uint64_t, std::string> RTXQueue::dropPacket (uint32_t seq)
{
    std::lock_guard<std::mutex> guard (m_mutex);
    auto el = m_map[seq];
    m_map.erase (seq);
    return el;
}

std::vector<uint32_t> RTXQueue::olderThan (uint64_t ts)
{
    std::lock_guard<std::mutex> guard (m_mutex);
    std::vector<uint32_t> ret;
    for (const auto& kv : m_map)
    {
	if (kv.second.first < ts)
	{
	    ret.push_back (kv.first);
	}
    }
    return ret;
}

std::vector<uint32_t> RTXQueue::unacked (uint64_t ts)
{
    std::lock_guard<std::mutex> guard (m_mutex);
    std::vector<uint32_t> ret;
    ret.reserve (m_map.size ());
    for (const auto& kv : m_map)
    {
	ret.push_back (kv.first);
    }
    return ret;
}

size_t RTXQueue::size () const
{
    return m_map.size ();
}
