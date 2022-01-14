////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2021 Vladislav Trifochkin
//
// This file is part of [net-lib](https://github.com/semenovf/net-lib) library.
//
// Changelog:
//      2021.10.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/endian.hpp"
#include "pfs/netty/p2p/udt/udp_socket.hpp"
#include <netinet/in.h>
#include <cassert>

#if NETTY_P2P__NEW_UDT_ENABLED
#   include "newlib/udt.hpp"
#else
#   include "lib/udt.h"
#endif

#include "debug_CCC.hpp"

namespace netty {
namespace p2p {
namespace udt {

static UDTSOCKET create (inet4_addr const & addr
    , std::uint16_t port
    , sockaddr_in & addr_in4)
{
    int ai_family   = AF_INET;    // AF_INET | AF_INET6
    int ai_socktype = SOCK_DGRAM; // SOCK_DGRAM | SOCK_STREAM
    int ai_protocol = 0;

    memset(& addr_in4, 0, sizeof(addr_in4));

    addr_in4.sin_family      = ai_family;
    addr_in4.sin_port        = pfs::to_network_order(static_cast<std::uint16_t>(port));
    addr_in4.sin_addr.s_addr = pfs::to_network_order(static_cast<std::uint32_t>(addr));

    UDTSOCKET socket = UDT::socket(ai_family, ai_socktype, ai_protocol);

    assert(socket != UDT::INVALID_SOCK);

    // UDT Options
    UDT::setsockopt(socket, 0, UDT_REUSEADDR, new bool(true), sizeof(bool));
    UDT::setsockopt(socket, 0, UDT_SNDSYN, new bool(false), sizeof(bool)); // sending is non-blocking
    UDT::setsockopt(socket, 0, UDT_RCVSYN, new bool(false), sizeof(bool)); // receiving is non-blocking
    //UDT::setsockopt(socket, 0, UDT_CC, new CCCFactory<debug_CCC>, sizeof(CCCFactory<debug_CCC>));

    //UDT::setsockopt(_socket, 0, UDT_MSS, new int(9000), sizeof(int));
    //UDT::setsockopt(_socket, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
    //UDT::setsockopt(_socket, 0, UDP_RCVBUF, new int(10000000), sizeof(int));

#if NETTY_P2P__NEW_UDT_ENABLED
    // TODO Need external configurable of bellow options
    UDT::setsockopt(socket, 0, UDT_EXP_MAX_COUNTER, new int(0), sizeof(int));
    UDT::setsockopt(socket, 0, UDT_EXP_THRESHOLD  , new std::uint64_t(1000000), sizeof(std::uint64_t));
#endif

    return socket;
}

udp_socket::~udp_socket ()
{
    close();
}

udp_socket::state_enum udp_socket::state () const
{
    assert(_socket >= 0);
    auto status = UDT::getsockstate(_socket);
    return static_cast<state_enum>(status);
}

bool udp_socket::bind (inet4_addr const & addr, std::uint16_t port)
{
    sockaddr_in addr_in4;
    _socket = create(addr, port, addr_in4);

    auto rc = UDT::bind(_socket
        , reinterpret_cast<sockaddr *>(& addr_in4)
        , sizeof(addr_in4));

    if (UDT::ERROR == rc) {
        failure(fmt::format("bind {}:{} to socket failure: {}"
            , to_string(addr)
            , port
            , error_string()));
        return false;
    }

    return true;
}

bool udp_socket::listen (int backlog)
{
    assert(_socket >= 0);

    auto rc = UDT::listen(_socket, backlog);

    if (rc < 0) {
        failure(fmt::format("`listen` failure: {}"
            , error_string()));
        return false;
    }

    return true;
}

udp_socket udp_socket::accept (inet4_addr * addr_ptr, std::uint16_t * port_ptr)
{
    udp_socket result;

    sockaddr saddr;
    int addrlen;

    result._socket = UDT::accept(_socket, & saddr, & addrlen);

    if (saddr.sa_family == AF_INET) {
        auto addr_in4_ptr = reinterpret_cast<sockaddr_in *>(& saddr);

        if (addr_ptr) {
            *addr_ptr = pfs::to_native_order(
                static_cast<std::uint32_t>(addr_in4_ptr->sin_addr.s_addr));
        }

        if (port_ptr) {
            *port_ptr = pfs::to_native_order(
                static_cast<std::uint16_t>(addr_in4_ptr->sin_port));
        }

        //UDT::setsockopt(result._socket, 0, UDT_LINGER
        //    , new linger{1, 3}, sizeof(linger));

    #if NETTY_P2P__NEW_UDT_ENABLED
        // TODO Need external configurable of bellow options
        UDT::setsockopt(result._socket, 0, UDT_EXP_MAX_COUNTER, new int(0), sizeof(int));
        UDT::setsockopt(result._socket, 0, UDT_EXP_THRESHOLD  , new std::uint64_t(1000000), sizeof(std::uint64_t));
    #endif

    } else {
        failure("`accept` failure: unsupported sockaddr family"
            " (AF_INET supported only)");
    }

    return result;
}

bool udp_socket::connect (inet4_addr const & addr, std::uint16_t port)
{
    sockaddr_in addr_in4;
    _socket = create(addr, port, addr_in4);

    auto rc = UDT::connect(_socket
        , reinterpret_cast<sockaddr *>(& addr_in4)
        , sizeof(addr_in4));

    if (rc < 0) {
        failure(fmt::format("connection to {}:{} failure: {}"
            , to_string(addr)
            , port
            , error_string()));
        return false;
    }

    return true;
}

void udp_socket::close ()
{
    if (_socket >= 0) {
        UDT::close(_socket);
        _socket = -1;
    }
}

std::vector<std::pair<std::string, std::string>> udp_socket::dump_options () const
{
    std::vector<std::pair<std::string, std::string>> result;
    int opt_size {0};

    // UDT_MSS - Maximum packet size (bytes). Including all UDT, UDP, and IP
    // headers. Default 1500 bytes.
    int mss {0};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_MSS, & mss, & opt_size));
    result.push_back(std::make_pair("UDT_MSS", std::to_string(mss)
        + ' ' + "bytes (max packet size)"));

    // UDT_SNDSYN - Synchronization mode of data sending. True for blocking
    // sending; false for non-blocking sending. Default true.
    bool sndsyn {false};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_SNDSYN, & sndsyn, & opt_size));
    result.push_back(std::make_pair("UDT_SNDSYN", sndsyn
        ? "TRUE (sending blocking)"
        : "FALSE (sending non-blocking)"));

    // UDT_RCVSYN - Synchronization mode for receiving.	true for blocking
    // receiving; false for non-blocking receiving. Default true.
    bool rcvsyn {false};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_RCVSYN, & rcvsyn, & opt_size));
    result.push_back(std::make_pair("UDT_RCVSYN", rcvsyn
        ? "TRUE (receiving blocking)"
        : "FALSE (receiving non-blocking)"));

    // UDT_FC - Maximum window size (packets). Default 25600.
    int fc {0};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_FC, & fc, & opt_size));
    result.push_back(std::make_pair("UDT_FC", std::to_string(fc)
        + ' ' + "packets (max window size)"));

    // UDT_STATE - Current status of the UDT socket.
    std::int32_t state {0};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_STATE, & state, & opt_size));
    result.push_back(std::make_pair("UDT_STATE", state_string(state)));

    // UDT_LINGER - Linger time on close().
    linger lng {0, 0};
    int linger_sz = sizeof(linger);
    assert(0 == UDT::getsockopt(_socket, 0, UDT_LINGER, & lng, & linger_sz));
    result.push_back(std::make_pair("UDT_LINGER"
        , "{" + std::to_string(lng.l_onoff)
        + ", " + std::to_string(lng.l_linger) + "}"));

    // UDT_EVENT - The EPOLL events available to this socket.
    std::int32_t event {0};
    assert(0 == UDT::getsockopt(_socket, 0, UDT_EVENT, & event, & opt_size));

    std::string event_str;

    if (event & UDT_EPOLL_IN)
        event_str += (event_str.empty() ? std::string{} : " | ") + "UDT_EPOLL_IN";
    if (event & UDT_EPOLL_OUT)
        event_str += (event_str.empty() ? std::string{} : " | ") + "UDT_EPOLL_OUT";
    if (event & UDT_EPOLL_ERR)
        event_str += (event_str.empty() ? std::string{} : " | ") + "UDT_EPOLL_ERR";

    result.push_back(std::make_pair("UDT_EVENT", event_str.empty() ? "<empty>" : event_str));

    return result;
}

std::streamsize udp_socket::recv (char * data, std::streamsize len)
{
    // FIXME Need more smart implementation
    return UDT::recvmsg(_socket, data, len);
}

std::streamsize udp_socket::send (char const * data, std::streamsize len)
{
    // FIXME Need more smart implementation

    int ttl_millis = -1;
    bool inorder = true;

    // Return
    // > 0             - on success;
    // = 0             - if UDT_SNDTIMEO > 0 and the message cannot be sent
    //                   before the timer expires;
    //                   if len <= 0.
    // UDT::ERROR (-1) - on error
    auto rc = UDT::sendmsg(_socket, data, len, ttl_millis, inorder);

    if (rc == UDT::ERROR) {
//         if ()
        return -1;
    }

    return rc;
}

std::string udp_socket::error_string () const
{
    return UDT::getlasterror().getErrorMessage();
}

std::string udp_socket::state_string (int state)
{
    switch (state) {
        case INIT      : return "INIT";
        case OPENED    : return "OPENED";
        case LISTENING : return "LISTENING";
        case CONNECTING: return "CONNECTING";
        case CONNECTED : return "CONNECTED";
        case BROKEN    : return "BROKEN";
        case CLOSING   : return "CLOSING";
        case CLOSED    : return "CLOSED";
        case NONEXIST  : return "NONEXIST";
    }

    return std::string{"<INVALID STATE>"};
}

}}} // namespace netty::p2p::udt
