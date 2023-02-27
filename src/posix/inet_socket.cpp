////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/endian.hpp"
#include "pfs/i18n.hpp"
#include "pfs/netty/error.hpp"
#include "pfs/netty/posix/inet_socket.hpp"

#if _MSC_VER
#   include <winsock2.h>
#   include <vector>
#else
#   include <sys/ioctl.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <string.h>
#   include <unistd.h>
#endif

namespace netty {
namespace posix {

inet_socket::native_type const inet_socket::kINVALID_SOCKET;

inet_socket::inet_socket () = default;

inet_socket::inet_socket (native_type sock, socket4_addr const & saddr)
    : _socket(sock)
    , _saddr(saddr)
{}

inet_socket::inet_socket (type_enum socktype)
{
    int ai_family = AF_INET;
    int ai_socktype = -1;
    int ai_protocol = 0;

    switch (socktype) {
        case type_enum::stream:
            ai_socktype = SOCK_STREAM;
            break;
        case type_enum::dgram:
            ai_socktype = SOCK_DGRAM;
            break;
        default:
            break;
    }

    if (ai_socktype < 0) {
        throw error {
              errc::socket_error
            , tr::_("bad/unsupported socket type")
        };
    }

#ifndef _MSC_VER
    ai_socktype |= SOCK_NONBLOCK;
#endif

    _socket = ::socket(ai_family, ai_socktype, ai_protocol);

    if (_socket < 0) {
        throw error {
              errc::socket_error
            , tr::_("create INET socket failure")
            , pfs::system_error_text()
        };
    }

#if _MSC_VER
    {
        u_long nonblocking = 1;
        auto rc = ::ioctlsocket(_socket, FIONBIO, & nonblocking);

        if (rc != 0) {
            throw error {
                  errc::socket_error
                , tr::_("create INET socket failure: set non-blocking")
                , pfs::system_error_text()
            };
        }
    }
#endif

    int yes = 1;
    int rc = 0;

#if defined(SO_REUSEADDR)
    if (rc == 0) {
#   if _MSC_VER
        rc = ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR
           , reinterpret_cast<char *>(& yes), sizeof(int));
#   else
        rc = ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, & yes, sizeof(int));
#   endif
    }
#endif

#if defined(SO_KEEPALIVE)
    if (ai_socktype & SOCK_STREAM) {
        if (rc == 0) {
#   if _MSC_VER
            rc = ::setsockopt(_socket, SOL_SOCKET, SO_KEEPALIVE
                , reinterpret_cast<char *>(& yes), sizeof(int));
#else
            rc = ::setsockopt(_socket, SOL_SOCKET, SO_KEEPALIVE, & yes, sizeof(int));
#endif
        }
    }
#endif

    if (rc != 0) {
        throw error {
              errc::socket_error
            , tr::_("set socket option failure")
            , pfs::system_error_text()
        };
    }
}

inet_socket::inet_socket (inet_socket && other)
{
    this->operator = (std::move(other));
}

inet_socket & inet_socket::operator = (inet_socket && other)
{
    this->~inet_socket();
    _socket = other._socket;
    _saddr  = other._saddr;
    other._socket = kINVALID_SOCKET;
    return *this;
}

inet_socket::~inet_socket ()
{
#if _MSC_VER
    if (_socket != kINVALID_SOCKET) {
        ::shutdown(_socket, SD_BOTH);
        ::closesocket(_socket);
#else
    if (_socket > 0) {
        ::shutdown(_socket, SHUT_RDWR);
        ::close(_socket);
#endif

        _socket = kINVALID_SOCKET;
    }
}

inet_socket::operator bool () const noexcept
{
    return _socket != kINVALID_SOCKET;
}

inet_socket::native_type inet_socket::native () const noexcept
{
    return _socket;
}

socket4_addr inet_socket::saddr () const noexcept
{
    return _saddr;
}

inline bool inet_socket::check_socket_descriptor (native_type sock, error * perr)
{
    if (sock == inet_socket::kINVALID_SOCKET) {
        error err { errc::invalid_argument, tr::_("bad socket descriptor") };

        if (perr) {
            *perr = std::move(err);
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

bool inet_socket::bind (native_type sock, socket4_addr const & saddr, error * perr)
{
    if (!check_socket_descriptor(sock, perr))
        return false;

    sockaddr_in addr_in4;

    std::memset(& addr_in4, 0, sizeof(addr_in4));

    addr_in4.sin_family      = AF_INET;
    addr_in4.sin_port        = pfs::to_network_order(static_cast<std::uint16_t>(saddr.port));
    addr_in4.sin_addr.s_addr = pfs::to_network_order(static_cast<std::uint32_t>(saddr.addr));

    auto rc = ::bind(sock
        , reinterpret_cast<sockaddr *>(& addr_in4)
        , sizeof(addr_in4));

    if (rc != 0) {
        error err {
              errc::socket_error
            , tr::f_("bind name to socket failure: {}", to_string(saddr))
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

bool inet_socket::set_nonblocking (bool enable, error * perr)
{
    return set_nonblocking(_socket, enable, perr);
}

bool inet_socket::set_nonblocking (native_type sock, bool enable, error * perr)
{
    if (!check_socket_descriptor(sock, perr))
        return false;

#if _MSC_VER
    unsigned long mode = enable ? 1 : 0;
    auto rc = ::ioctlsocket(fd, FIONBIO, & mode);

    if (rc != 0) {
#else
    int rc = ::fcntl(sock, F_GETFL, 0);

    if (rc >= 0) {
        rc = enable ? (rc | O_NONBLOCK) : (rc & ~O_NONBLOCK);
        rc = ::fcntl(sock, F_SETFL, rc);
    }

    if (rc < 0) {
#endif
        error err {
              errc::socket_error
            , tr::f_("set socket to {} mode failure"
                , enable ? tr::_("nonblocking") : tr::_("blocking"))
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

bool inet_socket::is_nonblocking (native_type sock, error * perr)
{
    if (!check_socket_descriptor(sock, perr))
        return false;

#if _MSC_VER
    // No "direct" way to determine mode of the socket.
    error err {
          errc::operation_not_permitted
        , tr::_("unable to determine socket mode on Windows")
        , pfs::system_error_text()
    };

    if (perr)
        *perr = std::move(err);
    else
        throw err;

    return false;
#else
    int rc = ::fcntl(sock, F_GETFL, 0);

    if (rc >= 0)
        return rc & O_NONBLOCK;

    if (rc < 0) {
        error err {
              errc::socket_error
            , tr::_("get socket flags failure")
            , pfs::system_error_text()
        };

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }

    return false;
#endif
}

int inet_socket::available (error * perr) const
{
#if _MSC_VER
    char peek_buffer[1500];
    std::vector<WSABUF> buf;
    DWORD n = 0;

    for (;;) {
        buf.resize(buf.size() + 5, {sizeof(peek_buffer), peek_buffer});

        DWORD flags = MSG_PEEK;
        DWORD bytes_read = 0;
        auto rc = ::WSARecv(_socket, buf.data(), DWORD(buf.size()), & bytes_read, & flags, nullptr, nullptr);

        if (rc != SOCKET_ERROR) {
            n = bytes_read;
            break;
        } else {
            auto lastWsaError = WSAGetLastError();

            switch (lastWsaError) {
                case WSAEMSGSIZE:
                    continue;
                case WSAECONNRESET:
                case WSAENETRESET:
                    n = 0;
                    break;
                default: {
                    error err {
                          errc::socket_error
                        , tr::_("read available data size from socket failure")
                        , pfs::system_error_text()
                    };

                    if (perr) {
                        *perr = std::move(err);
                        n = -1;
                    } else {
                        throw err;
                    }
                    break;
                }
            }

            break;
        }
    }

    return n;
#else
    // Check if any data available on socket
    std::uint8_t c;
    auto rc = ::recv(_socket, & c, 1, MSG_PEEK);

    if (rc < 0
        && errno == EAGAIN
                || (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK)) {
        return 0;
    }

    if (rc <= 0)
        return rc;

    int n = 0;
    auto rc1 = ::ioctl(_socket, FIONREAD, & n);

    if (rc1 != 0) {
        error err {
              errc::socket_error
            , tr::_("read available data size from socket failure")
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return -1;
        } else {
            throw err;
        }
    }

    return n;
#endif
}

int inet_socket::recv (char * data, int len, error * perr)
{
    auto n = ::recv(_socket, data, static_cast<int>(len), 0);

    if (n < 0) {
#if _MSC_VER
        auto lastWsaError = WSAGetLastError();

        if (lastWsaError == WSAEWOULDBLOCK) {
#else
        if (errno == EAGAIN || (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK)) {
#endif
            n = 0;
        } else {
            error err {
                  errc::socket_error
                , tr::_("receive data failure")
                , pfs::system_error_text()
            };

            if (perr) {
                *perr = std::move(err);
                return n;
            } else {
                throw err;
            }
        }
    }

    return n;
}

int inet_socket::recv_from (char * data, int len, socket4_addr * saddr, error * perr)
{
    sockaddr_in addr_in4;
    std::memset(& addr_in4, 0, sizeof(addr_in4));

#if _MSC_VER
    int addr_in4_len = sizeof(addr_in4);
    auto n = ::recvfrom(_socket, data, static_cast<int>(len), 0
        , reinterpret_cast<sockaddr *>(& addr_in4), & addr_in4_len);
#else
    socklen_t addr_in4_len = sizeof(addr_in4);
    auto n = ::recvfrom(_socket, data, static_cast<int>(len), 0
        , reinterpret_cast<sockaddr *>(& addr_in4), & addr_in4_len);
#endif

    if (n < 0) {
#if _MSC_VER
        auto lastWsaError = WSAGetLastError();

        if (lastWsaError == WSAEWOULDBLOCK) {
#else
        if (errno == EAGAIN || (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK)) {
#endif
            n = 0;
        } else {
            error err {
                  errc::socket_error
                , tr::_("receive data failure")
                , pfs::system_error_text()
            };

            if (perr) {
                *perr = std::move(err);
                 return n;
            } else {
                throw err;
            }
        }
    }


    if (saddr) {
        saddr->port = pfs::to_native_order(static_cast<std::uint16_t>(addr_in4.sin_port));
        saddr->addr = pfs::to_native_order(static_cast<std::uint32_t>(addr_in4.sin_addr.s_addr));
    }

    return n;
}

send_result inet_socket::send (char const * data, int len, error * perr)
{
    // MSG_NOSIGNAL flag means:
    // requests not to send SIGPIPE on errors on stream oriented sockets
    // when the other end breaks the connection.
    // The EPIPE error is still returned.
    //

#if _MSC_VER
    auto n = ::send(_socket, data, len, 0);
#else
    auto n = static_cast<decltype(send_result::n)>(::send(_socket
        , data, len, MSG_NOSIGNAL | MSG_DONTWAIT));
#endif

    if (n < 0) {
#if _MSC_VER
        auto lastWsaError = WSAGetLastError();

        if (lastWsaError == WSAENOBUFS)
            return send_result{send_status::overflow, n};

        if (lastWsaError == WSAECONNRESET || lastWsaError == WSAENETRESET
                || lastWsaError == WSAENETDOWN || lastWsaError == WSAENETUNREACH)
            return send_result{send_status::network, n};

        if (lastWsaError == WSAEWOULDBLOCK)
            return send_result{send_status::again, n};
#else
        // man send:
        // The output queue for a network interface was full. This generally
        // indicates that the interface has stopped sending, but may be
        // caused by transient congestion.(Normally, this does not occur in
        // Linux. Packets are just silently dropped when a device queue
        // overflows.)
        if (errno == ENOBUFS)
            return send_result{send_status::overflow, n};

        if (errno == ECONNRESET || errno == ENETRESET || errno == ENETDOWN
                || errno == ENETUNREACH)
            return send_result{send_status::network, n};

        if (errno == EAGAIN || (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK))
            return send_result{send_status::again, n};
#endif
        error err {
              errc::socket_error
            , tr::_("send failure")
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return send_result{send_status::failure, n};
        } else {
            throw err;
        }
    }

    return send_result{send_status::good, n};
}

// See inet_socket::send
send_result inet_socket::send_to (socket4_addr const & saddr
    , char const * data, int len, error * perr)
{
    sockaddr_in addr_in4;

    std::memset(& addr_in4, 0, sizeof(addr_in4));

    addr_in4.sin_family      = AF_INET;
    addr_in4.sin_port        = pfs::to_network_order(static_cast<std::uint16_t>(saddr.port));
    addr_in4.sin_addr.s_addr = pfs::to_network_order(static_cast<std::uint32_t>(saddr.addr));

#if _MSC_VER
    auto n = ::sendto(_socket, data, len, 0
        , reinterpret_cast<sockaddr *>(& addr_in4), sizeof(addr_in4));
#else
    auto n = static_cast<decltype(send_result::n)>(::sendto(_socket, data, len
        , MSG_NOSIGNAL | MSG_DONTWAIT
        , reinterpret_cast<sockaddr *>(& addr_in4), sizeof(addr_in4)));
#endif

    if (n < 0) {
#if _MSC_VER
        auto lastWsaError = WSAGetLastError();

        if (lastWsaError == WSAENOBUFS)
            return send_result{send_status::overflow, n};

        if (lastWsaError == WSAECONNRESET || lastWsaError == WSAENETRESET
            || lastWsaError == WSAENETDOWN || lastWsaError == WSAENETUNREACH)
            return send_result{send_status::network, n};

        if (lastWsaError == WSAEWOULDBLOCK)
            return send_result{send_status::again, n};

#else
        if (errno == ENOBUFS)
            return send_result{send_status::overflow, n};

        if (errno == ECONNRESET || errno == ENETRESET || errno == ENETDOWN
                || errno == ENETUNREACH)
            return send_result{send_status::network, n};

        if (errno == EAGAIN || (EAGAIN != EWOULDBLOCK && errno == EWOULDBLOCK))
            return send_result{send_status::again, n};
#endif
        error err {
              errc::socket_error
            , tr::f_("send to socket failure: {}", to_string(saddr))
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return send_result{send_status::failure, n};
        } else {
            throw err;
        }
    }

    return send_result{send_status::good, n};
}

}} // namespace netty::posix
