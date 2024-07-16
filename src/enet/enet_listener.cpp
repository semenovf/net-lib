////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2024.05.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "netty/enet/enet_listener.hpp"
#include <pfs/endian.hpp>
#include <pfs/i18n.hpp>
#include <enet/enet.h>

namespace netty {
namespace enet {

enet_listener::enet_listener ()
{}

enet_listener::enet_listener (socket4_addr const & saddr)
{
    _saddr = saddr;
}

enet_listener::enet_listener (socket4_addr const & saddr, int backlog)
    : enet_listener(saddr)
{
    listen(backlog);
}

bool enet_listener::listen (int backlog, error * perr)
{
    ENetAddress address;

    address.host = pfs::to_network_order(static_cast<enet_uint32>(_saddr.addr));
    address.port = _saddr.port;

    _host = enet_host_create(& address // the address to bind the server host to */
        , backlog // allow up to `backlog` clients and/or outgoing connections
        , 2       // allow up to 2 channels to be used, 0 and 1
        , 0       // assume any amount of incoming bandwidth
        , 0);     // assume any amount of outgoing bandwidth


    if (_host == nullptr) {
        pfs::throw_or(perr, error {
              errc::socket_error
            , tr::_("create ENet listener failure")
        });

        return false;
    }

    return true;
}

enet_listener::~enet_listener ()
{
    if (_host != nullptr)
        enet_host_destroy(_host);

    _host = nullptr;
}

enet_listener::native_type enet_listener::native () const noexcept
{
    if (_host == nullptr)
        return enet_socket::kINVALID_SOCKET;

    return reinterpret_cast<native_type>(_host);
}

enet_socket enet_listener::accept (error * perr)
{
    return accept(reinterpret_cast<native_type>(_host), perr);
}

enet_socket enet_listener::accept_nonblocking (error * perr)
{
    return accept_nonblocking(reinterpret_cast<native_type>(_host), perr);
}

// [static]
enet_socket enet_listener::accept (native_type listener_sock , error * /*perr*/)
{
    return enet_socket{listener_sock};
}

// [static]
enet_socket enet_listener::accept_nonblocking (native_type listener_sock, error * /*perr*/)
{
    return enet_socket{listener_sock};
}

}} // namespace netty::enet
