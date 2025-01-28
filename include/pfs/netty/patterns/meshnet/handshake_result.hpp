////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file is part of `netty-lib`.
//
// Changelog:
//      2025.01.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <pfs/netty/namespace.hpp>

NETTY__NAMESPACE_BEGIN

namespace patterns {
namespace meshnet {

enum class handshake_result_enum
{
      unusable = 0 // Socket is unusable, may be closed
    , reader = 1   // Use socket as reader
    , writer = 2   // Use socket as writer
};

}} // namespace patterns::meshnet

NETTY__NAMESPACE_END
