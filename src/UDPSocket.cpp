#include <sys/socket.h>

#include "UDPSocket.hpp"
#include "Util.hpp"
#include "Timing.hpp"

using namespace std;

/* default constructor for socket of (subclassed) domain and type */
UDPSocket::UDPSocket( void )
    : FileDescriptor( SystemCall( "socket", socket( AF_INET, SOCK_DGRAM, 0 ) ) )
{
/* allow local address to be reused sooner, at the cost of some robustness */
    setsockopt ( SOL_SOCKET, SO_REUSEADDR, int(true));

/* turn on timestamps on receipt */
#if defined (SO_TIMESTAMPNS)
    setsockopt (SOL_SOCKET, SO_TIMESTAMPNS, int(true));
#elif defined (SO_TIMESTAMP)
    setsockopt (SOL_SOCKET, SO_TIMESTAMP, int(true));
#else
# error "Cannot get packet timestamps."
#endif

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = UDPSocket::Timeout;
    setsockopt (SOL_SOCKET, SO_RCVTIMEO, tv);
}

/* bind socket to a specified local address (usually to listen/accept) */
void UDPSocket::bind( const Address & address )
{
    sockaddr addr = address.to_sockaddr ();
    socklen_t asize = address.size ();
    SystemCall( "bind", ::bind( fd_num(), &addr, asize));
}

static uint64_t packetTimestamp (msghdr *hdr)
{
    cmsghdr *tshdr = CMSG_FIRSTHDR (hdr);

#if defined (SO_TIMESTAMPNS)
    int desired = SCM_TIMESTAMPNS;
#elif defined (SO_TIMESTAMP)
    int desired = SCM_TIMESTAMP;
#else
# error "Cannot get packet timestamps."
#endif

    uint64_t timestamp = 0;
    while (tshdr)
    {
	if (tshdr->cmsg_type != desired)
	{
	    fprintf (stderr, "Unexpected message type 0x%x.\n", (uint32_t)tshdr->cmsg_type);
	    abort ();
	}
	else
	{
#if defined (SO_TIMESTAMPNS)
	    const timespec *tsp = reinterpret_cast<timespec *>(CMSG_DATA (tshdr));
	    timestamp = ((tsp->tv_sec * 1000000000) + tsp->tv_nsec) / 1000;
#elif defined (SO_TIMESTAMP)
	    const timeval *tvp = reinterpret_cast<timeval *>(CMSG_DATA (tshdr));
	    timestamp = (tvp->tv_sec * 1000000) + tvp->tv_usec;
#else
#error "Cannot get packet timestamps."
#endif
	}
	tshdr = CMSG_NXTHDR (hdr, tshdr);
    }

    if (!timestamp)
    {
	fprintf (stderr, "Did not find packet timestamp.\n");
	abort ();
    }

    return timestamp;
}


// pkt is allocated by the caller
Address UDPSocket::recv (Packet &pkt)
{
    static const ssize_t RECEIVE_MTU = 65536;

    /* receive source address, timestamp and payload */
    sockaddr datagram_source_address;
    msghdr header; zero( header );
    iovec msg_iovec; zero( msg_iovec );

    char msg_control[ RECEIVE_MTU ];

    /* prepare to get the payload */
    msg_iovec.iov_base = pkt.buf ();
    msg_iovec.iov_len = Packet::MTU + 4;

    /* prepare to get the source address */
    header.msg_name = &datagram_source_address;
    header.msg_namelen = sizeof( datagram_source_address );

    header.msg_iov = &msg_iovec;
    header.msg_iovlen = 1;

    /* prepare to get the timestamp */
    header.msg_control = msg_control;
    header.msg_controllen = sizeof( msg_control );

    /* call recvmsg */
    ssize_t recv_len = recvmsg (fd_num(), &header, 0);

//  if ((recv_len == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    if (recv_len == -1)
    {
	//timeout
	return Address ();
    }
    register_read();

    /* make sure we got the whole datagram */
    if ( header.msg_flags & MSG_TRUNC )
    {
	fprintf (stderr, "recvfrom (oversized datagram)\n");
	abort ();
    } else if ( header.msg_flags )
    {
	fprintf (stderr, "recvfrom (unhandled flag)\n");
	abort ();
    }

    pkt.size (recv_len);
    pkt.tsRecv (packetTimestamp (&header));
  
    return Address (datagram_source_address, header.msg_namelen);
      
}


/* send datagram to specified address */
void UDPSocket::sendto( const Address & destination, const void *buf, size_t size)
{
    const ssize_t bytes_sent =
	SystemCall( "sendto", ::sendto( fd_num(),
					buf,
					size,
					0,
					&destination.to_sockaddr(),
					destination.size() ) );

    register_write();

    if ( size_t( bytes_sent ) != size ) {
	fprintf (stderr, "datagram payload too big for sendto()\n" );
	exit (1);
    }
}

/* set socket option */
template <typename option_type>
void UDPSocket::setsockopt (const int level, const int option, const option_type & option_value)
{
    SystemCall( "setsockopt", ::setsockopt( fd_num(), level, option,
					    &option_value, sizeof( option_value ) ) );
}

