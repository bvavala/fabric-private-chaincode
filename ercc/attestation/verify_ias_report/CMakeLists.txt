# Copyright 2019 Intel Corporation
# Copyright IBM Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

set(COMMON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../common)

IF("${SGX_MODE}" STREQUAL "SIM")
    SET(IAS_CA_CERT_REQUIRED_FLAGS "")
ELSE()
    SET(IAS_CA_CERT_REQUIRED_FLAGS "-DIAS_CA_CERT_REQUIRED")
ENDIF()


set(SOURCE_FILES
    ias-certificates.cpp
    verify-report.cpp
    ${COMMON_SOURCE_DIR}/c11_support.cpp
    )

set(cleanup_files
    ${CMAKE_CURRENT_SOURCE_DIR}/ias-certificates.cpp)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${cleanup_files}")

#add_custom_command(
#    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/enclave_u.c
#    COMMAND ${SGX_EDGER8R} --untrusted enclave.edl
#    --search-path ../enclave/ # enclave.edl
#    --search-path ${COMMON_SOURCE_DIR}/enclave
#    --search-path ${SGX_SDK}/include
#    COMMENT "Generating enclave_u.{c,h}"
#    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#    )

add_library(verifyiasreport SHARED ${SOURCE_FILES})

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${COMMON_SOURCE_DIR}
    ${SGX_SDK}/include
    )

TARGET_COMPILE_OPTIONS(verifyiasreport PRIVATE ${IAS_CA_CERT_REQUIRED_FLAGS} "-fpie" "-fPIC" "-fstack-protector" "-std=c++11" "-Wall")

#set_target_properties(libverifyiasreport PROPERTIES PREFIX "")

#target_link_libraries(libverifyiasreport
#    -L${SGX_LIBRARY_PATH} -l${SGX_URTS_LIB} -l${SGX_USVC_LIB} -L${SGX_SSL_LIBRARY_PATH} -lsgx_usgxssl)

#add_custom_command(TARGET libsgxcc
#    POST_BUILD
#    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/include
#    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/sgxcclib.h ${CMAKE_BINARY_DIR}/include
#    COMMAND cp ${COMMON_SOURCE_DIR}/types.h ${CMAKE_BINARY_DIR}/include
#    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/lib/
#    COMMAND cp ${CMAKE_BINARY_DIR}/libsgxcc.so ${CMAKE_BINARY_DIR}/lib/
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#    COMMENT "Add include to build")

