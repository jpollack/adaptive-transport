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
    void bind( const Address & address );

    /* accessors */
    Address local_address() const;
    Address peer_address() const;

    /* receive datagram, timestamp, and where it came from into a preallocated structure */
    Address recv (Packet &);
    
    void sendto (const Address & peer, const void* buf, size_t size);
private:
    /* get the local or peer address the socket is connected to */
    Address get_address( const std::string & name_of_function,
			 const std::function<int(int, sockaddr *, socklen_t *)> & function ) const;

    /* set socket option */
    template <typename option_type>
    void setsockopt (const int level, const int option, const option_type& option_value);
    /* allow local address to be reused sooner, at the cost of some robustness */
    void set_reuseaddr();
    void set_timeout (uint64_t usecs);
    /* turn on timestamps on receipt */
    void set_timestamps();

};

