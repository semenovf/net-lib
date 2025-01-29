////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2025.01.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/netty/namespace.hpp>
#include <functional>
#include <string>

NETTY__NAMESPACE_BEGIN

namespace patterns {
namespace meshnet {

template <typename Node>
struct functional_callbacks
{
    // Notify when connection established with the remote node
    std::function<void(typename Node::node_id id)> on_node_ready = [] (typename Node::node_id) {};
};

}} // namespace patterns::meshnet

NETTY__NAMESPACE_END

