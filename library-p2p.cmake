################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `netty-lib`.
#
# Changelog:
#      2021.06.21 Initial version.
#      2021.06.22 Fixed completely.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(netty-p2p CXX)

option(NETTY_P2P__ENABLE_QT5 "Enable Qt5 library (network backend)" OFF)
option(NETTY_P2P__ENABLE_QT6 "Enable Qt6 library (network backend)" OFF)
option(NETTY_P2P__ENABLE_UDT "Enable UDT library (reliable UDP implementation)" OFF)
option(NETTY_P2P__ENABLE_NEW_UDT "Enable modified UDT library (reliable UDP implementation)" ON)
option(NETTY_P2P__UDT_PATCHED "Enable modified UDT library with patches" ON)
option(NETTY_P2P__ENABLE_CEREAL "Enable cereal library (serialization backend)" ON)
option(NETTY_P2P__ENABLE_CEREAL_THREAD_SAFETY "Enable cereal library thread safety" OFF)

portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::netty::p2p
    EXPORTS NETTY__EXPORTS
    BIND_STATIC ${PROJECT_NAME}-static
    STATIC_ALIAS pfs::netty::p2p::static
    STATIC_EXPORTS NETTY__STATIC)

portable_target(DEFINITIONS ${PROJECT_NAME} PRIVATE UDT_EXPORTS)
portable_target(DEFINITIONS ${PROJECT_NAME}-static PUBLIC UDT_STATIC)

if (NETTY_P2P__ENABLE_QT5)
    portable_target(LINK_QT5_COMPONENTS ${PROJECT_NAME} PUBLIC Core Network)
    portable_target(LINK_QT5_COMPONENTS ${PROJECT_NAME}-static PUBLIC Core Network)

    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "NETTY_P2P__QT5_CORE_ENABLED=1")
    portable_target(DEFINITIONS ${PROJECT_NAME}-static PUBLIC "NETTY_P2P__QT5_CORE_ENABLED=1")

    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "NETTY_P2P__QT5_NETWORK_ENABLED=1")
    portable_target(DEFINITIONS ${PROJECT_NAME}-static PUBLIC "NETTY_P2P__QT5_NETWORK_ENABLED=1")
endif(NETTY_P2P__ENABLE_QT5)

if (NETTY_P2P__ENABLE_UDT)
    set(NETTY_P2P__UDT_ENABLED ON CACHE BOOL "UDT enabled")
    set(NETTY_P2P__UDT_ROOT "${CMAKE_CURRENT_LIST_DIR}/src/udt/lib")
    portable_target(DEFINITIONS ${PROJECT_NAME} PRIVATE "NETTY_P2P__UDT_ENABLED=1")
    portable_target(DEFINITIONS ${PROJECT_NAME}-static PRIVATE "NETTY_P2P__UDT_ENABLED=1")
endif(NETTY_P2P__ENABLE_UDT)

if (NETTY_P2P__ENABLE_NEW_UDT)
    set(NETTY_P2P__UDT_ENABLED ON CACHE BOOL "modified UDT enabled")
    set(NETTY_P2P__UDT_ROOT "${CMAKE_CURRENT_LIST_DIR}/src/udt/newlib")
    portable_target(DEFINITIONS ${PROJECT_NAME} PRIVATE "NETTY_P2P__NEW_UDT_ENABLED=1")
    portable_target(DEFINITIONS ${PROJECT_NAME}-static PRIVATE "NETTY_P2P__NEW_UDT_ENABLED=1")
endif(NETTY_P2P__ENABLE_NEW_UDT)

if (NETTY_P2P__UDT_PATCHED)
    portable_target(DEFINITIONS ${PROJECT_NAME} PRIVATE "NETTY_P2P__UDT_PATCHED=1")
    portable_target(DEFINITIONS ${PROJECT_NAME}-static PRIVATE "NETTY_P2P__UDT_PATCHED=1")
endif()

if (NETTY_P2P__ENABLE_CEREAL)
    if (NOT TARGET cereal)
        portable_target(INCLUDE_PROJECT ${CMAKE_CURRENT_LIST_DIR}/cmake/Cereal.cmake)
    endif()
    portable_target(LINK ${PROJECT_NAME} PUBLIC cereal)
    portable_target(LINK ${PROJECT_NAME}-static PUBLIC cereal)
endif(NETTY_P2P__ENABLE_CEREAL)

portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::netty)
portable_target(LINK ${PROJECT_NAME}-static PUBLIC pfs::netty::static)

if (MSVC)
    portable_target(LINK ${PROJECT_NAME} PUBLIC ws2_32)
    portable_target(LINK ${PROJECT_NAME}-static PUBLIC ws2_32)
endif(MSVC)

if (NETTY_P2P__UDT_ENABLED OR NETTY_P2P__ENABLE_NEW_UDT)
    # UDT sources
    portable_target(SOURCES ${PROJECT_NAME}
        ${NETTY_P2P__UDT_ROOT}/api.cpp
        ${NETTY_P2P__UDT_ROOT}/buffer.cpp
        ${NETTY_P2P__UDT_ROOT}/cache.cpp
        ${NETTY_P2P__UDT_ROOT}/ccc.cpp
        ${NETTY_P2P__UDT_ROOT}/channel.cpp
        ${NETTY_P2P__UDT_ROOT}/common.cpp
        ${NETTY_P2P__UDT_ROOT}/core.cpp
        ${NETTY_P2P__UDT_ROOT}/epoll.cpp
        ${NETTY_P2P__UDT_ROOT}/list.cpp
        ${NETTY_P2P__UDT_ROOT}/md5.cpp
        ${NETTY_P2P__UDT_ROOT}/packet.cpp
        ${NETTY_P2P__UDT_ROOT}/queue.cpp
        ${NETTY_P2P__UDT_ROOT}/window.cpp)

    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/udt/api.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/udt/poller.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/udt/udp_socket.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/udt/debug_CCC.cpp)
endif()

if (MSVC)
    portable_target(COMPILE_OPTIONS ${PROJECT_NAME} "/wd4251")
    portable_target(COMPILE_OPTIONS ${PROJECT_NAME}-static "/wd4251")
endif(MSVC)
