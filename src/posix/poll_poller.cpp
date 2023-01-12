////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/netty/error.hpp"
#include "pfs/netty/posix/poll_poller.hpp"
#include <algorithm>
#include <string.h>
#include <sys/socket.h>

namespace netty {
namespace posix {

poll_poller::poll_poller ()
{}

poll_poller::~poll_poller ()
{}

void poll_poller::add (native_socket_type sock, error * perr)
{
    (void)perr;

    events.push_back(pollfd{});
    auto & ev = events.back();
    ev.fd = sock;
    ev.revents = 0;
    ev.events = POLLERR | POLLHUP | POLLIN | POLLOUT;

#ifdef POLLRDHUP
    ev.events |= POLLRDHUP;
#endif

#ifdef POLLRDNORM
    ev.events |= POLLRDNORM;
#endif

#ifdef POLLRDBAND
    ev.events |= POLLRDBAND;
#endif

#ifdef POLLWRNORM
    ev.events |= POLLWRNORM;
#endif

#ifdef POLLWRBAND
    ev.events |= POLLWRBAND;
#endif
}

void poll_poller::remove (native_socket_type sock, error * perr)
{
    (void)perr;

    auto pos = std::find_if(events.begin(), events.end()
        , [& sock] (pollfd const & p) { return sock == p.fd;});

    if (pos != events.end()) {
        // pollfd is POD, so we can simply shift tail to the left and do resize
        // instead of call `erase` method
        auto index = std::distance(events.begin(), pos);
        auto ptr = events.data();
        auto sz = events.size();
        auto n = sz - 1 - index;

        memcpy(ptr + index, ptr + index + 1, n);
        events.resize(sz - 1);
    }
}

int poll_poller::poll (std::chrono::milliseconds millis, error * perr)
{
    auto n = ::poll(events.data(), events.size(), millis.count());

    if (n < 0) {
        error err {
              make_error_code(errc::poller_error)
            , tr::_("poll failure")
            , pfs::system_error_text()
        };

        if (perr) {
            *perr = std::move(err);
            return n;
        } else {
            throw err;
        }
    }

    return n;
}

bool poll_poller::empty () const noexcept
{
    return events.size() == 0;
}

}} // namespace netty::posix
