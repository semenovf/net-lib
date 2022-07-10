////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// License: see LICENSE file
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2021.10.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/universal_id.hpp"
#include "pfs/netty/p2p/envelope.hpp"
#include "pfs/netty/p2p/packet.hpp"

namespace p2p = netty::p2p;

namespace {
    constexpr std::uint16_t PACKET_SIZE = 64;
}

using universal_id      = pfs::universal_id;
using packet_t          = p2p::packet;
using output_envelope_t = p2p::output_envelope<>;
using input_envelope_t  = p2p::input_envelope<>;

TEST_CASE("packet_serialization")
{
    packet_t pkt;
    auto sender_uuid = "01FH7H6YJB8XK9XNNZYR0WYDJ1"_uuid;
    std::string payload {"Hello, World!"};

    p2p::split_into_packets(PACKET_SIZE, sender_uuid
        , payload.data(), payload.size(), [& pkt] (packet_t && p) {
            pkt = std::move(p);
        });

    output_envelope_t oe;
    oe.seal(pkt);

    auto data = oe.data();

    CHECK((data.size() == packet_t::MAX_PACKET_SIZE));

    input_envelope_t ie {data};

    ie.unseal(pkt);

    CHECK(pkt.packetsize == PACKET_SIZE);
    CHECK_EQ(pkt.uuid, sender_uuid);
    CHECK_EQ(pkt.partcount, 1);
    CHECK_EQ(pkt.partindex, 1);
    CHECK_EQ(pkt.payloadsize, 13);
    CHECK_EQ(payload, std::string(pkt.payload, pkt.payloadsize));
}
