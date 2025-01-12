/*******************************************************************************
 * Copyright (C) 2016-2018, SoftPLC Corporation.
 *
 ******************************************************************************/

#ifndef CIPSTER_SOCKADDR_H_
#define CIPSTER_SOCKADDR_H_

#include <string.h>
#include <string>

#if defined(__linux__)
 #include <errno.h>
 #include <arpa/inet.h>
#elif defined(_WIN32)
 #undef _WINSOCKAPI_    // suppress Mingw32's "Please include winsock2.h before windows.h"
 #include <winsock2.h>
 #include <ws2tcpip.h>
#else
 #include <netinet/in.h>
 #include <errno.h>
#endif

#include <stdexcept>


const int SADDRZ = sizeof(sockaddr);

std::string IpAddrStr( in_addr aIP );


class socket_error : public std::runtime_error
{
public:
    socket_error( const std::string& aMessage ) :
        std::runtime_error( aMessage ),
#if defined(__linux__)
        error_code( errno )
#elif defined(_WIN32)
        error_code( WSAGetLastError() )
#else
        error_code( errno )
#endif
    {}

    socket_error( const std::string& aMessage, int aError ) :
        std::runtime_error( aMessage ),
        error_code( aError )
    {}

    int error_code;
};


/**
 * Class SockAddr
 * is a wrapper for a sock_addr_in.  It provides host endian accessors so that
 * client code can mostly forget about network endianess.  It also provides an
 * operator to convert itself directly into a (sockaddr_in*) for use in BSD
 * sockets calls.
 *
 * @see #Cpf which knows how to serialize and deserialize this for its
 *   own needs, and on the wire it is called a "SockAddr Info Item".
 */
class SockAddr
{
public:
    SockAddr(
            unsigned aPort = 0,
            unsigned aIP = INADDR_ANY       // INADDR_ANY is zero
            );

    SockAddr( const sockaddr_in& aSockAddr ) :
        sa( aSockAddr )
    {}

    SockAddr( const char* aNameOrIPAddr, unsigned aPort );

    /// assign from a sockaddr_in to this
    SockAddr& operator=( const sockaddr_in& rhs )
    {
        sa = rhs;
        return *this;
    }

    bool operator==( const SockAddr& other ) const
    {
        return sa.sin_addr.s_addr == other.sa.sin_addr.s_addr
           &&  sa.sin_port == other.sa.sin_port;
    }

    bool operator!=( const SockAddr& other ) const  { return !(*this == other); }

    operator const sockaddr_in& () const    { return sa; }
    operator const sockaddr*    () const    { return (sockaddr*) &sa; }
    operator       sockaddr*    () const    { return (sockaddr*) &sa; }

    // All accessors take and return host endian values.  Internally,
    // sin_port and sin_addr.s_addr are stored in network byte order (big endian).

    SockAddr&   SetFamily( unsigned aFamily )   { sa.sin_family = aFamily;                  return *this; }
    SockAddr&   SetPort( unsigned aPort )       { sa.sin_port = htons( aPort );             return *this; }
    SockAddr&   SetAddr( unsigned aIPAddr )     { sa.sin_addr.s_addr = htonl( aIPAddr );    return *this; }

    unsigned Family() const     { return sa.sin_family; }
    unsigned Port() const       { return ntohs( sa.sin_port ); }
    unsigned Addr() const       { return ntohl( sa.sin_addr.s_addr ); }

    std::string AddrStr() const { return IpAddrStr( sa.sin_addr ); }

    /**
     * Function IsValid
     * checks fields according to CIP Vol2 3-3.9.4
     * and returns true if valid, else false.
     */
    bool IsValid() const
    {
        return sa.sin_family == AF_INET
          &&   !memcmp( &sa.sin_zero[0], &sa.sin_zero[1], 7 )
          &&   !sa.sin_zero[0];
    }

    bool IsMulticast() const
    {
        return (0xf0000000 & Addr()) == 0xe0000000;
        // Vol2 3-5.3
        // https://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml
        // "The multicast addresses are in the range 224.0.0.0 through 239.255.255.255."
    }

protected:
     sockaddr_in    sa;
};

#endif  //  CIPSTER_SOCKADDR_H_
