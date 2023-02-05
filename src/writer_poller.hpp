////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/i18n.hpp"
#include "pfs/netty/writer_poller.hpp"

namespace netty {

template <typename Backend>
writer_poller<Backend>::writer_poller ()
    : writer_poller(specialized{})
{
    on_error = [] (native_socket_type, std::string const & text) {
        fmt::print(stderr, tr::_("ERROR: writer poller: {}\n"), text);
    };
}

template <typename Backend>
writer_poller<Backend>::~writer_poller () = default;

template <typename Backend>
void writer_poller<Backend>::add (native_socket_type sock, error * perr)
{
    _rep.add(sock, perr);
}

template <typename Backend>
void writer_poller<Backend>::remove (native_socket_type sock, error * perr)
{
    _rep.remove(sock, perr);
}

template <typename Backend>
bool writer_poller<Backend>::empty () const noexcept
{
    return _rep.empty();
}

} // namespace netty
