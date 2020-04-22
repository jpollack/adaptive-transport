#pragma once
#include <cstdint>

struct Packet
{
public:
    static constexpr uint32_t MTU = 1400;
    Packet ();
    ~Packet ();

    // ptr to entire packet.  R/W; caller can copy new packet directly here
    // then use accessors to parse
    char *buf (void);

    // Size of the data payload.  Wire size -- includes the header byte(s).
    const uint16_t size (void) const;
    void size (uint16_t);

    // Timestamp packet was received.
    const uint64_t tsRecv (void) const;
    void tsRecv (uint64_t);

    // Timestamp packet was sent.
    const uint64_t tsSent (void) const;
    void tsSent (uint64_t);

    // Timestamp packet was acked.
    const uint64_t tsAck (void) const;
    void tsAck (uint64_t);
    
    // These accessors directly mutate data in m_buf.  
    // Is Ack?
    const bool isAck (void) const;
    void isAck (bool);

    // Sequence number
    const uint32_t seq (void) const;
    void seq (uint32_t);

    // // Segment number -- only valid if data packet
    // const uint32_t seg (void) const;
    // void seg (uint32_t);

    // Data payload -- only valid if data packet
    char *dataPayload (void);

    // ACK tsRecv -- only valid if ack packet.
    const uint64_t tsRecvAck (void) const;
    void tsRecvAck (uint64_t);
    
private:
    char m_buf[MTU + 4];
    uint16_t m_size;
    uint64_t m_tsSent;
    uint64_t m_tsRecv;
    uint64_t m_tsAck;
};
