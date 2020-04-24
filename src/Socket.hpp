#pragma once
#include <functional>

#include "Address.hpp"
#include "FileDescriptor.hpp"
#include "Packet.hpp"

/* class for network sockets (UDP, TCP, etc.) */
class UDPSocket : public FileDescriptor
{
public:
    /* default constructor */
    UDPSocket(void);
    static constexpr uint32_t Timeout = 10000; // microseconds

    /* bind socket to a specified local address (usually to listen/accept) */
    void bind (const Address & address);

    /* receive datagram, timestamp, and where it came from into a preallocated structure */
    Address recv (Packet &);
    
    void sendto (const Address & peer, const void* buf, size_t size);
private:

    /* set socket option */
    template <typename option_type>
    void setsockopt (const int level, const int option, const option_type& option_value);

};

