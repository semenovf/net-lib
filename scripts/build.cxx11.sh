#!/bin/bash

CMAKE_OPTIONS="-DNETTY_P2P__ENABLE_QT5=ON \
-DNETTY_P2P__ENABLE_NEW_UDT=ON \
-DNETTY_P2P__ENABLE_CEREAL=ON"

PROJECT_OPT_PREFIX=NETTY__
CXX_STANDARD=11
BUILD_TESTS=ON
BUILD_DEMO=ON

. ./build.sh

echo $?