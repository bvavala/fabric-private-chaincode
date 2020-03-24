# Copyright IBM Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.5.1)

include(../../cmake/Init.cmake)

include(../../cmake/ConfigSGX.cmake)

#set(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../common)

add_subdirectory(verify_ias_report)

set(BUILD_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/include ${CMAKE_BINARY_DIR}/lib)
