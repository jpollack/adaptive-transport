#pragma once

#include "UDPEndpoint.hpp"
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
#include "RTXQueue.hpp"

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
    std::function<void(uint32_t seq)> onDroppedFunc;
    std::function<void(uint32_t seq, uint64_t tsSent, uint64_t tsRecv)> onAckedFunc;

    uint32_t bandwidth;
    uint32_t mtu;
    std::atomic<uint64_t> rtt;
    
private:
    struct Payload
    {
	bool reliable;
	bool bwlimited;
	std::string payload;
    };
    
    using PayloadQueue = moodycamel::BlockingConcurrentQueue<Payload>;
    RTXQueue m_rtxq;
    PayloadQueue m_sendQueue;
    PTT m_ptt;
    bool m_done;
    uint32_t m_nextSeq;
    CircBuff m_recvBuff;
    uint32_t m_dseq;
    std::thread m_sender;
    std::thread m_receiver;
    std::thread m_controller;
    std::thread m_limiter;
    std::atomic<uint32_t> m_bytesQueued;
    void senderEntry (void);
    void receiverEntry (void);
    void controllerEntry (void);
    void limiterEntry (void);
    void limiterStep (void);
    uint64_t getDelay (uint32_t size);
    void enqueueSend (const std::string& payload, bool reliable);
    void onRecvData (uint32_t dseq, const char *base, uint32_t size);
    uint64_t onRecvAck (uint32_t seq, uint64_t tsRecv, uint64_t ackRecv);
    uint64_t m_tsLast;
    uint32_t m_bytesLast;
    
    std::unordered_map<uint32_t,std::string> m_recvMap;
    UDPSocket m_socket;
    Address m_peer;
};
