#pragma once

#include "Address.hpp"
#include "PTT.hpp"
#include "readerwriterqueue.h"
#include "blockingconcurrentqueue.h"
#include <unordered_map>
#include "CircBuff.hpp"
#include "Socket.hpp"
#include "Address.hpp"
#include <atomic>
#include <functional>
#include "RollingStats.hpp"
#include "Packet.hpp"
#include "PacketQueue.hpp"

/* This should implement packetiszing of the data, retransmits, and timing */

class UDPStream
{
public:
    UDPStream ();
    ~UDPStream ();

    // uint8_t *wptr ();
    // void wptrAdvance (size_t num = 1);

    void send (const std::string& msg);
    
    const uint8_t* rptr ();
    void rptrAdvance (size_t num = 1);
    uint32_t readable (void);
    
    void setLocal (const Address& addr);
    void setRemote (const Address& addr);
    uint32_t bytesQueued (void) const;
    // allows for easily printing timing information.  HACK.
    std::function<void(uint32_t seq, uint64_t tsSent, uint64_t tsRecv)> onPacketMetadata;

    uint32_t bandwidth;
    uint32_t mtu;
    RollingStats rtt;
    bool updateBandwidth;

private:
    PacketQueue m_packetQueue;
    PTT m_ptt;
    bool m_done;
    uint32_t m_nextSeq;
    CircBuff m_recvBuff;
    uint32_t m_dseq;
    std::thread m_sender;
    std::thread m_receiver;
    std::thread m_limiter;
    uint64_t m_tsUpdated;
    void senderEntry (void);
    void receiverEntry (void);
    int retransmit (void);
    void limiterEntry (void);
    std::tuple<int,int,int,int> limiterStats (void);
    void onRecvData (uint32_t dseq, const char *base, uint32_t size);
    void onMetadata (uint32_t seq, uint64_t tsSent, uint64_t tsRecv, uint64_t tsAck);
    std::unordered_map<uint32_t,std::string> m_recvMap;
    UDPSocket m_socket;
    Address m_peer;
};
