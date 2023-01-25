////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2021 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/netty/posix/tcp_socket.hpp"
#include "pfs/endian.hpp"
#include "pfs/i18n.hpp"
#include "pfs/netty/error.hpp"
#include <memory>

#if _MSC_VER
#   include <winsock2.h>
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#endif

namespace netty {
namespace posix {

tcp_socket::tcp_socket () : inet_socket(type_enum::stream) {}

tcp_socket::tcp_socket (uninitialized) : inet_socket() {}

// Accepted socket
tcp_socket::tcp_socket (native_type sock, socket4_addr const & saddr)
    : inet_socket(sock, saddr)
{}

tcp_socket::tcp_socket (tcp_socket && other)
    : inet_socket(std::move(other))
{}

tcp_socket & tcp_socket::operator = (tcp_socket && other)
{
    inet_socket::operator = (std::move(other));
    return *this;
}

tcp_socket::~tcp_socket () = default;

conn_status tcp_socket::connect (socket4_addr const & saddr, error * perr)
{
    sockaddr_in addr_in4;

    memset(& addr_in4, 0, sizeof(addr_in4));

    addr_in4.sin_family      = AF_INET;
    addr_in4.sin_port        = pfs::to_network_order(static_cast<std::uint16_t>(saddr.port));
    addr_in4.sin_addr.s_addr = pfs::to_network_order(static_cast<std::uint32_t>(saddr.addr));

    auto rc = ::connect(_socket
        , reinterpret_cast<sockaddr *>(& addr_in4)
        , sizeof(addr_in4));

    bool in_progress = false;

    if (rc < 0) {
        in_progress = (errno == EINPROGRESS);

        if (!in_progress) {
            error err {
                  errc::socket_error
                , tr::_("socket connect error")
                , pfs::system_error_text()
            };

            if (perr) {
                *perr = std::move(err);
            } else {
                throw err;
            }

            return conn_status::failure;
        }
    }

    _saddr = saddr;

    return in_progress ? conn_status::connecting : conn_status::connected;
}

void tcp_socket::disconnect (error * perr)
{
    if (_socket > 0) {
        auto rc = ::shutdown(_socket
#if _MSC_VER
            , SD_BOTH);
#else
            , SHUT_RDWR);
#endif

        if (rc != 0) {
            auto ec = pfs::get_last_system_error();

#if _MSC_VER
            if (ec.value() != WSAENOTCONN) {
#else // _MSC_VER
            if (ec.value() != ENOTCONN) {
#endif // POSIX
                error err {
                      errc::socket_error
                    , tr::_("socket shutdown error")
                    , pfs::system_error_text()
                };

                if (perr) {
                    *perr = std::move(err);
                } else {
                    throw err;
                }
            }
        }
    }
}

}} // namespace netty::posix
