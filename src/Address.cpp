#include <string>
#include <cstring>
#include <memory>

#include <netdb.h>
#include <algorithm>
#include "Address.hpp"
#include "Util.hpp"


Address::Address (const sockaddr& other, socklen_t otherlen)
{
    m_addr = other;
    m_addrlen = otherlen;
}

std::pair<std::string, uint16_t> Address::ip_port() const
{
  char ip[ NI_MAXHOST ], port[ NI_MAXSERV ];

  const int gni_ret = getnameinfo(&to_sockaddr(),
				  size (),
                                   ip, sizeof( ip ),
                                   port, sizeof( port ),
                                   NI_NUMERICHOST | NI_NUMERICSERV );
  if ( gni_ret ) {
      fprintf (stderr, "getnameinfo: %s\n", gai_strerror (gni_ret));
      exit (1);
  }

  /* attempt to shorten v4-mapped address if possible */
  std::string ip_string { ip };
  if ( ip_string.size() > 7
       and ip_string.substr( 0, 7 ) == "::ffff:" ) {
      const std::string candidate_ipv4 = ip_string.substr( 7 );
      Address candidate_addr( candidate_ipv4, port );
    if ( candidate_addr == *this ) {
      ip_string = candidate_ipv4;
    }
  }

  return make_pair( ip_string, std::stoi( port ) );
}

std::string Address::to_string() const
{
    const auto [ ip, port ] = ip_port ();
    return ip + ":" + std::to_string (port);
}

const sockaddr &Address::to_sockaddr() const
{
    return m_addr;
}

socklen_t Address::size () const
{
    return m_addrlen;
}


/* equality */
bool Address::operator==( const Address & other ) const
{
    return 0 == memcmp( &m_addr, &other.m_addr, sizeof(sockaddr) );
}

Address::Address (void)
    : m_addrlen (0)
{}

Address::Address (const std::string& hostname, const std::string& service)
{
    addrinfo *addri;
    addrinfo hints;

    memset (&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;
    if (std::any_of (hostname.begin (), hostname.end (), [](const char c){ return (c < '.') || (c > '9'); }))
    {
	// requires resolution
	hints.ai_flags &= (~AI_NUMERICHOST);
    }
    
    const int ret = getaddrinfo (hostname.c_str (), service.c_str (), &hints, &addri);

    if (ret)
    {
	fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (ret));
	exit (1);
    }

    m_addr = *addri->ai_addr;
    m_addrlen = addri->ai_addrlen;
    
    freeaddrinfo (addri);
}
