# Copyright 2019 Intel Corporation
# Copyright IBM Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.5.1)

set(SOURCE_FILES
    battleship_cc.cpp
    battleship-core/battleship-core.cpp
    )

include_directories(
    battleship-core
    )

include(../../ecc_enclave/enclave/CMakeLists-common-app-enclave.txt)

