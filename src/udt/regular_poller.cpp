////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2023 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2023.01.09 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "../regular_poller.hpp"

#if NETTY__UDT_ENABLED
#   include "newlib/udt.hpp"
#   include "pfs/netty/udt/epoll_poller.hpp"
#endif

#include "pfs/log.hpp"

static constexpr char const * TAG = "UDT";

namespace netty {

#if NETTY__UDT_ENABLED

template <typename Backend>
int regular_poller<Backend>::poll (std::chrono::milliseconds millis, error * perr)
{
    auto n = _rep.poll(_rep.eid, & _rep.readfds, & _rep.writefds, millis, perr);

    if (n < 0)
        return n;

    if (n > 0) {
        if (!_rep.readfds.empty()) {
            for (UDTSOCKET u: _rep.readfds) {
                auto state = UDT::getsockstate(u);
                //LOGD(TAG, "UDT read socket state: {}", status);

                if (state == CONNECTED || state == OPENED) {
                    if (ready_read)
                        ready_read(u);
                } else {
                    auto disconnect = false;

                    if (state == CLOSED) {
                        disconnect = true;
                    } else {
                        disconnect = true;
                        on_error(u, tr::f_("read socket failure: state={}"
                            " (TODO: handle properly)", state));
                    }

                    if (disconnect) {
                        remove(u);

                        if (disconnected)
                            disconnected(u);
                    }
                }
            }
        }

        if (!_rep.writefds.empty()) {
            for (UDTSOCKET u: _rep.writefds) {
                //auto state = UDT::getsockstate(u);
                //LOGD(TAG, "UDT write socket state: {}", state);

                if (can_write)
                    can_write(u);
            }
        }
    }

    return n;
}

#endif // NETTY__UDT_ENABLED

#if NETTY__UDT_ENABLED
template class regular_poller<udt::epoll_poller>;
#endif

} // namespace netty
