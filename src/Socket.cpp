#include <sys/socket.h>

#include "Socket.hpp"
#include "Util.hpp"
#include "Timing.hpp"

using namespace std;

/* default constructor for socket of (subclassed) domain and type */
Socket::Socket( const int domain, const int type )
  : FileDescriptor( SystemCall( "socket", socket( domain, type, 0 ) ) )
{}

/* construct from file descriptor */
Socket::Socket( FileDescriptor && fd, const int domain, const int type )
  : FileDescriptor( move( fd ) )
{
  int actual_value;
  socklen_t len;

  /* verify domain */
  len = sizeof( actual_value );
  SystemCall( "getsockopt",
	      getsockopt( fd_num(), SOL_SOCKET, SO_DOMAIN, &actual_value, &len ) );
  if ( (len != sizeof( actual_value )) or (actual_value != domain) ) {
      fprintf (stderr, "socket domain mismatch\n");
      exit (1);
  }

  /* verify type */
  len = sizeof( actual_value );
  SystemCall( "getsockopt",
	      getsockopt( fd_num(), SOL_SOCKET, SO_TYPE, &actual_value, &len ) );
  if ( (len != sizeof( actual_value )) or (actual_value != type) ) {
      fprintf (stderr, "socket type mismatch\n");
      exit (1);
  }
}

/* get the local or peer address the socket is connected to */
Address Socket::get_address( const std::string & name_of_function,
			     const std::function<int(int, sockaddr *, socklen_t *)> & function ) const
{
    sockaddr addr;
    socklen_t size = sizeof(sockaddr);

  SystemCall( name_of_function, function( fd_num(),
					  &addr,
					  &size ) );

  return Address (addr, size);
}

Address Socket::local_address() const
{
  return get_address( "getsockname", getsockname );
}

Address Socket::peer_address() const
{
  return get_address( "getpeername", getpeername );
}

/* bind socket to a specified local address (usually to listen/accept) */
void Socket::bind( const Address & address )
{
    sockaddr addr = address.to_sockaddr ();
    socklen_t asize = address.size ();
    SystemCall( "bind", ::bind( fd_num(), &addr, asize));
}

/* connect socket to a specified peer address */
void Socket::connect( const Address & address )
{
  SystemCall( "connect", ::connect( fd_num(),
				    &address.to_sockaddr(),
				    address.size() ) );
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
  msg_iovec.iov_len = Packet::MTU + 8;

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


/* receive datagram and where it came from */
std::tuple<uint64_t,Address,std::string> UDPSocket::recv (void)
{
  static const ssize_t RECEIVE_MTU = 65536;

  /* receive source address, timestamp and payload */
  sockaddr datagram_source_address;
  msghdr header; zero( header );
  iovec msg_iovec; zero( msg_iovec );

  char msg_payload[ RECEIVE_MTU ];
  char msg_control[ RECEIVE_MTU ];

  /* prepare to get the source address */
  header.msg_name = &datagram_source_address;
  header.msg_namelen = sizeof( datagram_source_address );

  /* prepare to get the payload */
  msg_iovec.iov_base = msg_payload;
  msg_iovec.iov_len = sizeof( msg_payload );
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
      return std::make_tuple (MicrosecondsSinceEpoch (), Address (), "");
  }
  register_read();

  /* make sure we got the whole datagram */
  if ( header.msg_flags & MSG_TRUNC ) {
      fprintf (stderr, "recvfrom (oversized datagram)\n");
      abort ();
  } else if ( header.msg_flags ) {
      fprintf (stderr, "recvfrom (unhandled flag)\n");
      abort ();
  }

  uint64_t timestamp = packetTimestamp (&header);
  return std::make_tuple (timestamp,
			  Address (datagram_source_address, header.msg_namelen),
			  std::string (msg_payload, recv_len));
  
}

/* send datagram to specified address */
void UDPSocket::sendto( const Address & destination, const string & payload )
{
    const ssize_t bytes_sent =
    SystemCall( "sendto", ::sendto( fd_num(),
				    payload.data(),
				    payload.size(),
				    0,
				    &destination.to_sockaddr(),
				    destination.size() ) );

  register_write();

  if ( size_t( bytes_sent ) != payload.size() ) {
      fprintf (stderr, "datagram payload too big for sendto()\n" );
      exit (1);
  }
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

/* send datagram to connected address */
void UDPSocket::send( const string & payload )
{
  const ssize_t bytes_sent =
    SystemCall( "send", ::send( fd_num(),
				payload.data(),
				payload.size(),
				0 ) );

  register_write();

  if ( size_t( bytes_sent ) != payload.size() ) {
      fprintf (stderr,  "datagram payload too big for send()\n" );
      exit (1);
  }
}

/* mark the socket as listening for incoming connections */
void TCPSocket::listen( const int backlog )
{
  SystemCall( "listen", ::listen( fd_num(), backlog ) );
}

/* accept a new incoming connection */
TCPSocket TCPSocket::accept()
{
  register_read();
  return TCPSocket( FileDescriptor( SystemCall( "accept", ::accept( fd_num(), nullptr, nullptr ) ) ) );
}

/* set socket option */
template <typename option_type>
void Socket::setsockopt (const int level, const int option, const option_type & option_value)
{
  SystemCall( "setsockopt", ::setsockopt( fd_num(), level, option,
					  &option_value, sizeof( option_value ) ) );
}

/* allow local address to be reused sooner, at the cost of some robustness */
void Socket::set_reuseaddr()
{
  setsockopt( SOL_SOCKET, SO_REUSEADDR, int( true ) );
}

void Socket::set_timeout (uint64_t usecs)
{
    struct timeval tv;
    tv.tv_sec = usecs / 1000000;
    tv.tv_usec = usecs % 1000000;
    setsockopt (SOL_SOCKET, SO_RCVTIMEO, tv);
}

/* turn on timestamps on receipt */
void UDPSocket::set_timestamps()
{
#if defined (SO_TIMESTAMPNS)
    setsockopt( SOL_SOCKET, SO_TIMESTAMPNS, int( true ) );
#elif defined (SO_TIMESTAMP)
    setsockopt( SOL_SOCKET, SO_TIMESTAMP, int( true ) );
#else
    # error "Cannot get packet timestamps."
#endif
}

