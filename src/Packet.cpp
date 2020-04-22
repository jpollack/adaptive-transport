#include "Packet.hpp"
#include <cstring>

Packet::Packet ()
    :m_size (0),
     m_tsSent (0),
     m_tsRecv (0),
     m_tsAck (0)
{
    // memset (m_buf, 0, sizeof(m_buf));
}

Packet::~Packet ()
{
}

const bool Packet::isAck (void) const
{
    return !!(reinterpret_cast<const uint32_t *>(m_buf)[0] & (1 << 31));
}

void Packet::isAck (bool val)
{
    reinterpret_cast<uint32_t *>(m_buf)[0] = (reinterpret_cast<uint32_t *>(m_buf)[0] & ~(1 << 31)) | (val << 31);
}

const uint32_t Packet::seq (void) const
{
    return reinterpret_cast<const uint32_t *>(m_buf)[0] & ~(1 << 31);
}

void Packet::seq (uint32_t val)
{
    reinterpret_cast<uint32_t *>(m_buf)[0] = (reinterpret_cast<uint32_t *>(m_buf)[0] & (1 << 31)) | (val & ~(1 << 31));
}

// const uint32_t Packet::seg (void) const
// {
//     return reinterpret_cast<const uint32_t *>(m_buf)[1];
// }

// void Packet::seg (uint32_t val)
// {
//     reinterpret_cast<uint32_t *>(m_buf)[1] = val;
// }

char *Packet::dataPayload (void)
{
    return m_buf + 4;
}

char *Packet::buf (void)
{
    return m_buf;
}

const uint64_t Packet::tsRecvAck (void) const
{
    return reinterpret_cast<const uint64_t *>(m_buf + 4)[0];
}

void Packet::tsRecvAck (uint64_t val)
{
    reinterpret_cast<uint64_t *>(m_buf + 4)[0] = val;
}

const uint64_t Packet::tsRecv (void) const
{
    return m_tsRecv;
}

void Packet::tsRecv (uint64_t val)
{
    m_tsRecv = val;
}

const uint64_t Packet::tsSent (void) const
{
    return m_tsSent;
}

void Packet::tsSent (uint64_t val)
{
    m_tsSent = val;
}

const uint64_t Packet::tsAck (void) const
{
    return m_tsAck;
}

void Packet::tsAck (uint64_t val)
{
    m_tsAck = val;
}

const uint16_t Packet::size (void) const
{
    return m_size;
}

void Packet::size (uint16_t val)
{
    m_size = val;
}
// constexpr int Packet::MTU = 1400;
