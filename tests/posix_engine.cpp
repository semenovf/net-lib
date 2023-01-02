////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// License: see LICENSE file
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2021.10.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define PFS__LOG_LEVEL 3
#include "posix_engine.hpp"
#include "pfs/error.hpp"
#include "pfs/fmt.hpp"
#include "pfs/log.hpp"
#include <atomic>
#include <thread>


static char loremipsum[] =
"1.Lorem ipsum dolor sit amet, consectetuer adipiscing elit,    \n\
2.sed diam nonummy nibh euismod tincidunt ut laoreet dolore     \n\
3.magna aliquam erat volutpat. Ut wisi enim ad minim veniam,    \n\
4.quis nostrud exerci tation ullamcorper suscipit lobortis      \n\
5.nisl ut aliquip ex ea commodo consequat. Duis autem vel eum   \n\
6.iriure dolor in hendrerit in vulputate velit esse molestie    \n\
7.consequat, vel illum dolore eu feugiat nulla facilisis at     \n\
8.vero eros et accumsan et iusto odio dignissim qui blandit     \n\
9.praesent luptatum zzril delenit augue duis dolore te feugait  \n\
10.nulla facilisi. Nam liber tempor cum soluta nobis eleifend    \n\
11.option congue nihil imperdiet doming id quod mazim placerat   \n\
12.facer possim assum. Typi non habent claritatem insitam; est   \n\
13.usus legentis in iis qui facit eorum claritatem.              \n\
14.Investigationes demonstraverunt lectores legere me lius quod  \n\
15.ii legunt saepius. Claritas est etiam processus dynamicus,    \n\
16.qui sequitur mutationem consuetudium lectorum. Mirum est      \n\
17.notare quam littera gothica, quam nunc putamus parum claram,  \n\
18.anteposuerit litterarum formas humanitatis per seacula quarta \n\
19.decima et quinta decima. Eodem modo typi, qui nunc nobis      \n\
20.videntur parum clari, fiant sollemnes in futurum.             \n\
21.Lorem ipsum dolor sit amet, consectetuer adipiscing elit,     \n\
22.sed diam nonummy nibh euismod tincidunt ut laoreet dolore     \n\
23.magna aliquam erat volutpat. \"Ut wisi enim ad minim veniam,  \n\
24.quis nostrud exerci tation ullamcorper suscipit lobortis      \n\
25.nisl ut aliquip ex ea commodo consequat. Duis autem vel eum   \n\
26.iriure dolor in hendrerit in vulputate velit esse molestie    \n\
27.consequat, vel illum dolore eu feugiat nulla facilisis at     \n\
28.vero eros et accumsan et iusto odio dignissim qui blandit     \n\
29.praesent luptatum zzril delenit augue duis dolore te feugait  \n\
30.nulla facilisi. Nam liber tempor cum soluta nobis eleifend    \n\
31.option congue nihil imperdiet doming id quod mazim placerat   \n\
32.facer possim assum. Typi non habent claritatem insitam; est   \n\
33.usus legentis in iis qui facit eorum claritatem.              \n\
34.Investigationes demonstraverunt lectores legere me lius quod  \n\
35.ii legunt saepius. Claritas est etiam processus dynamicus,    \n\
36.qui sequitur mutationem consuetudium lectorum. Mirum est      \n\
37.notare quam littera gothica, quam nunc putamus parum claram,  \n\
38.anteposuerit litterarum formas humanitatis per seacula quarta \n\
39.decima et quinta decima.\" Eodem modo typi, qui nunc nobis    \n\
40.videntur parum clari, fiant sollemnes in futurum.";

namespace p2p = netty::p2p;

static constexpr std::size_t PACKET_SIZE = netty::p2p::packet::MAX_PACKET_SIZE; // = 1430

namespace {
    pfs::universal_id const PEER1_UUID = "01FH7H6YJB8XK9XNNZYR0WYDJ1"_uuid;
    pfs::universal_id const PEER2_UUID = "01FH7HB19B9T1CTKE5AXPTN74M"_uuid;

    std::chrono::milliseconds const DISCOVERY_TRANSMIT_INTERVAL {100};
    std::chrono::milliseconds const PEER_EXPIRATION_TIMEOUT {500};
    std::chrono::milliseconds const POLL_INTERVAL {1000};

    std::atomic_bool QUIT_PEER1 {false};
    std::atomic_bool QUIT_PEER2 {false};
}

void on_failure (std::string const & error)
{
    fmt::print(stderr, "!ERROR: {}\n", error);
}

void on_rookie_accepted (pfs::universal_id uuid
    , netty::inet4_addr const & addr
    , std::uint16_t port)
{
    LOG_TRACE_1("HELO: {} ({}:{})"
        , to_string(uuid)
        , to_string(addr)
        , port);
}

void on_writer_ready (pfs::universal_id uuid
    , netty::inet4_addr const & addr
    , std::uint16_t port)
{
    LOG_TRACE_1("WRITER READY: {} ({}:{})"
        , to_string(uuid)
        , to_string(addr)
        , port);

    if (uuid == PEER2_UUID)
        QUIT_PEER2 = true;
}

void on_peer_closed (pfs::universal_id uuid
    , netty::inet4_addr const & addr
    , std::uint16_t port)
{
    LOG_TRACE_1("CLOSED: {} ({}:{})"
        , to_string(uuid)
        , to_string(addr)
        , port);

    QUIT_PEER1 = true;
}

void worker (engine_t & peer)
{
        // FIXME
//     LOG_TRACE_1("Peer started: {}", to_string(peer.uuid()));

//     try {
//         while (true) {
//             peer.loop();
//
//             if (peer.uuid() == PEER1_UUID && QUIT_PEER1)
//                 break;
//
//             if (peer.uuid() == PEER2_UUID && QUIT_PEER2)
//                 break;
//         }
//     } catch (cereal::Exception ex) {
//         LOGE("cereal::Exception", "{} (peer={})", ex.what(), peer.uuid());
//     } catch (...) {
//         LOGE("Unhandled exception", "peer={}", peer.uuid());
//     }

//     LOG_TRACE_1("Peer finished: {}", peer.uuid());
}

void term_handler ()
{
    fmt::print(stderr, "TERMINATED\n");

    // FIXME
//     std::exception_ptr exptr = std::current_exception();
//     try {
//         std::rethrow_exception(exptr);
//     } catch (CUDTException ex) {
//         LOG_TRACE_1("!!! EXCEPTION: {} [{}]"
//         , ex.getErrorMessage()
//         , ex.getErrorCode());
//     }
}

int main ()
{
    std::set_terminate(term_handler);

    std::thread peer1_worker;
    std::thread peer2_worker;

    // FIXME
//     peer1_worker = std::thread{[] {
//         engine_t peer1 {PEER1_UUID};
//         peer1.failure = on_failure;
//         peer1.peer_discovered = on_rookie_accepted;
//         peer1.writer_ready = on_writer_ready;
//         peer1.peer_closed = on_peer_closed;
//
//         bool success = true;
//
//         success = success
//             && peer1.set_option(engine_t::option_enum::discovery_address
//                 , netty::socket4_addr{netty::inet4_addr{127, 0, 0, 1}, 5555u})
//             && peer1.set_option(engine_t::option_enum::transmit_interval
//                 , DISCOVERY_TRANSMIT_INTERVAL)
//             && peer1.set_option(engine_t::option_enum::expiration_timeout
//                 , PEER_EXPIRATION_TIMEOUT)
//             && peer1.set_option(engine_t::option_enum::poll_interval
//                 , POLL_INTERVAL)
//             && peer1.set_option(engine_t::option_enum::listener_address
//                 , netty::socket4_addr{netty::inet4_addr{127, 0, 0, 1}, 5556u})
//             && peer1.set_option(engine_t::option_enum::listener_backlog, 10);
//
//         if (peer1.start()) {
//             peer1.add_discovery_target(netty::inet4_addr{127, 0, 0, 1}, 7777u);
//             worker(peer1);
//         }
//     }};
//
//     peer2_worker = std::thread{[] {
//         engine_t peer2 {PEER2_UUID};
//         peer2.failure = on_failure;
//         peer2.peer_discovered = on_rookie_accepted;
//         peer2.writer_ready = on_writer_ready;
//         peer2.peer_closed = on_peer_closed;
//
//         bool success = true;
//         success = success
//             && peer2.set_option(engine_t::option_enum::discovery_address
//                 , netty::socket4_addr{netty::inet4_addr{127, 0, 0, 1}, 7777u})
//             && peer2.set_option(engine_t::option_enum::transmit_interval
//                 , DISCOVERY_TRANSMIT_INTERVAL)
//             && peer2.set_option(engine_t::option_enum::expiration_timeout
//                 , PEER_EXPIRATION_TIMEOUT)
//             && peer2.set_option(engine_t::option_enum::poll_interval
//                 , POLL_INTERVAL)
//             && peer2.set_option(engine_t::option_enum::listener_address
//                 , netty::socket4_addr{netty::inet4_addr{127, 0, 0, 1}, 7778u})
//             && peer2.set_option(engine_t::option_enum::listener_backlog, 10);
//
//         if (peer2.start()) {
//             peer2.add_discovery_target(netty::inet4_addr{127, 0, 0, 1}, 5555u);
//             worker(peer2);
//         }
//     }};

    peer1_worker.join();
    peer2_worker.join();

    return 0;
}

