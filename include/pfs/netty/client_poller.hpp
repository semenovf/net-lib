////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chrono.hpp"
#include "error.hpp"
#include "conn_status.hpp"
#include "connecting_poller.hpp"
#include "regular_poller.hpp"
#include "pfs/i18n.hpp"
#include <functional>

namespace netty {

template <typename Backend>
class client_poller
{
public:
    using native_socket_type = typename Backend::native_socket_type;

    struct callbacks
    {
        std::function<void(native_socket_type, std::string const &)> on_error;
        std::function<void(native_socket_type)> connection_refused;
        std::function<void(native_socket_type)> connected;
        std::function<void(native_socket_type)> ready_read;
        std::function<void(native_socket_type)> can_write;
    };

private:
    connecting_poller<Backend> _connecting_poller;
    regular_poller<Backend>    _poller;

private:
    std::function<void(native_socket_type, std::string const &)> on_error;
    std::function<void(native_socket_type)> connection_refused;
    std::function<void(native_socket_type)> connected;
    std::function<void(native_socket_type)> ready_read;
    std::function<void(native_socket_type)> can_write;

public:
    client_poller (callbacks && cbs)
        : _connecting_poller()
        , _poller()
    {
        if (cbs.on_error) {
            _connecting_poller.on_error = cbs.on_error;
            _poller.on_error = std::move(cbs.on_error);
        }

        _connecting_poller.connection_refused = std::move(cbs.connection_refused);
        _connecting_poller.connected = std::move(cbs.connected);

        _poller.ready_read = std::move(cbs.ready_read);
        _poller.can_write  = std::move(cbs.can_write);

    }

    ~client_poller () = default;

    client_poller (client_poller const &) = delete;
    client_poller & operator = (client_poller const &) = delete;
    client_poller (client_poller &&) = delete;
    client_poller & operator = (client_poller &&) = delete;

    /**
     * Add socket to connecting or regular poller according to it's connection
     * status @a cs.
     */
    void add (native_socket_type sock, conn_status state, error * perr = nullptr)
    {
        if (state == conn_status::connecting)
            _connecting_poller.add(sock, perr);
        else if (state == conn_status::connected)
            _poller.add(sock, perr);
        else {
            error err {
                  make_error_code(errc::poller_error)
                , tr::_("socket must be in a connecting or connected state to be"
                    " added to the client poller")
            };

            if (perr) {
                *perr = std::move(err);
                return;
            } else {
                throw err;
            }
        }
    }

    /**
     * Remove sockets from connecting and regular pollers.
     */
    void remove (native_socket_type sock, error * perr = nullptr)
    {
        _connecting_poller.remove(sock, perr);
        _poller.remove(sock, perr);
    }

    /**
     * Check if connecting and regular pollers are empty.
     */
    bool empty () const noexcept
    {
        return _connecting_poller.empty() && _poller.empty();
    }

    void poll (std::chrono::milliseconds millis, error * perr = nullptr)
    {
        if (!_connecting_poller.empty()) {
            if (_poller.empty())
                _connecting_poller.poll(millis, perr);
            else
                _connecting_poller.poll(std::chrono::milliseconds{0}, perr);
        }

        _poller.poll(millis);
    }
};

} // namespace netty
