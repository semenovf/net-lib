////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.09 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "../regular_poller.hpp"

#if NETTY__SELECT_ENABLED
#   include "pfs/netty/posix/select_poller.hpp"
#endif

#if NETTY__POLL_ENABLED
#   include "pfs/netty/posix/poll_poller.hpp"
#endif

#include <sys/socket.h>

namespace netty {

#if NETTY__SELECT_ENABLED

template <>
int regular_poller<posix::select_poller>::poll (std::chrono::milliseconds millis, error * perr)
{
    fd_set rfds;
    fd_set wfds;

    auto n = _rep.poll(& rfds, & wfds, millis, perr);

    if (n < 0)
        return n;

    if (n > 0) {
        int rcounter = n;

        for (int fd = _rep.min_fd; fd <= _rep.max_fd && rcounter > 0; fd++) {
            if (FD_ISSET(fd, & rfds)) {
                if (ready_read)
                    ready_read(fd);

                --rcounter;
            }

            if (FD_ISSET(fd, & wfds)) {
                if (can_write)
                    can_write(fd);

                --rcounter;
            }
        }
    }

    return n;
}

#endif // NETTY__SELECT_ENABLED

#if NETTY__POLL_ENABLED

template <>
int regular_poller<posix::poll_poller>::poll (std::chrono::milliseconds millis, error * perr)
{
    auto n = _rep.poll(millis, perr);

    if (n < 0)
        return n;

    if (n > 0) {
        for (int i = 0; i < n; i++) {
            auto revents = _rep.events[i].revents;
            auto fd = _rep.events[i].fd;

            // Writing is now possible, though a write larger than the available space
            // in a socket or pipe will still block (unless O_NONBLOCK is set).
            // Identical to `epoll_poller`
            if (revents & (POLLOUT
#ifdef POLLWRNORM
                | POLLWRNORM
#endif
#ifdef POLLWRBAND
                | POLLWRBAND
#endif
            )) {
                if (ready_read)
                    ready_read(fd);
            }

            // There is data to read - can accept
            // Identical to `epoll_poller`
            if (revents & (POLLIN
#ifdef POLLRDNORM
                | POLLRDNORM
#endif
#ifdef POLLRDBAND
                | POLLRDBAND
#endif
            )) {
                if (can_write)
                    can_write(fd);
            }
        }
    }

    return n;
}

#endif // NETTY__POLL_ENABLED

#if NETTY__SELECT_ENABLED
template class regular_poller<posix::select_poller>;
#endif

#if NETTY__POLL_ENABLED
template class regular_poller<posix::poll_poller>;
#endif

} // namespace netty
