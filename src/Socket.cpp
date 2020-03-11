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

    // void *p = &addr;
    // printf ("addr == ");
    // for (auto ii = 0; ii < asize; ++ii)
    // {
    // 	uint8_t c = *(const uint8_t *)(p + ii);
    // 	printf ("%02x ", c);
    // }
    // printf ("\nsize == %d\n", asize);
    
    SystemCall( "bind", ::bind( fd_num(), &addr, asize));
}

/* connect socket to a specified peer address */
void Socket::connect( const Address & address )
{
  SystemCall( "connect", ::connect( fd_num(),
				    &address.to_sockaddr(),
				    address.size() ) );
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
      exit (1);
  } else if ( header.msg_flags ) {
      fprintf (stderr, "recvfrom (unhandled flag)\n");
      exit (1);
  }

  uint64_t timestamp = -1;

  /* find the timestamp header (if there is one) */
  cmsghdr *ts_hdr = CMSG_FIRSTHDR( &header );
  while ( ts_hdr ) {
    if ( ts_hdr->cmsg_level == SOL_SOCKET
	 and ts_hdr->cmsg_type == SO_TIMESTAMPNS ) {
      const timespec * const kernel_time = reinterpret_cast<timespec *>( CMSG_DATA( ts_hdr ) );
      timestamp = ((kernel_time->tv_sec * 1000000000) + kernel_time->tv_nsec) / 1000;
    }
    ts_hdr = CMSG_NXTHDR( &header, ts_hdr );
  }

  return std::make_tuple (timestamp,
			  Address (datagram_source_address, header.msg_namelen),
			  std::string (msg_payload, recv_len));
  
}

/* send datagram to specified address */
void UDPSocket::sendto( const Address & destination, const string & payload )
{
    // const void *p = &destination.to_sockaddr ();
    // int dsize = destination.size ();
    // printf ("sending to addr[%d]: ", dsize);
    // for (auto ii = 0; ii < dsize; ++ii)
    // {
    // 	uint8_t c = *(const uint8_t *)(p + ii);
    // 	printf ("%02x ", c);
    // }

    // printf ("(%s)\n", destination.to_string ().c_str ());
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
void Socket::setsockopt( const int level, const int option, const option_type & option_value )
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
  setsockopt( SOL_SOCKET, SO_TIMESTAMPNS, int( true ) );
}

