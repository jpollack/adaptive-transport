#pragma once
#include <string>
#include <utility>

#include <netinet/in.h>
#include <netdb.h>

/* Address class for IPv4/IPv6 addresses */
class Address
{

public:
  /* constructors */
    Address (void);
    Address (const sockaddr& other, socklen_t otherlen);
    Address (const std::string& hostname, const std::string& service);

    const sockaddr &to_sockaddr() const;
    socklen_t size () const;
    /* accessors */
  std::pair<std::string, uint16_t> ip_port() const;
  std::string ip() const { return ip_port().first; }
  uint16_t port() const { return ip_port().second; }
  std::string to_string() const;

    
  /* equality */
  bool operator==( const Address & other ) const;

private:
    socklen_t m_addrlen;
    sockaddr m_addr;
    
};
