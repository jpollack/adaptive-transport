#pragma once

#include "Address.hpp"
#include "Socket.hpp"
#include "readerwriterqueue.h"
#include <future>
#include "PTT.hpp"
#include <functional>

// class TimedPacket
// {
// public:
//     uint64_t timestamp;
//     std::promise<uint64_t> timestampProm;
//     Address addr;
//     std::string payload;
// };

// using TimedPacketQueue = moodycamel::BlockingReaderWriterQueue<TimedPacket>;
// using PacketQueue = moodycamel::BlockingReaderWriterQueue<std::tuple<uint64_t,Address,std::string> >;

class UDPEndpoint
{
private:
    using PacketQueue = moodycamel::BlockingReaderWriterQueue<std::pair<uint32_t,std::string> >;
public:

    UDPEndpoint (PTT& ptt);
    ~UDPEndpoint ();

    // returns time sent at.  call PTT.seq() before his to get the sequence number.
    uint64_t sendAt (const std::string& payload, uint64_t when = 0);
    std::pair<uint32_t,std::string> queueRecv (void);
    void setLocal (const Address& addr);
    void setPeer (const Address& addr);
private:
    UDPSocket m_socket;
    bool m_done;
    PTT& m_ptt;
    std::thread m_receiver;
    std::vector<std::pair<uint32_t,uint64_t> > m_tsRecv;
    Address m_peer;
    PacketQueue m_recvQueue;

    std::pair<uint32_t,std::string> recv (void);
    void receiverEntry (void);

};
