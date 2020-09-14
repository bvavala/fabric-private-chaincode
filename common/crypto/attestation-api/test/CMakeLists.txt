# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE ("$ENV{FPC_PATH}/cmake/ConfigSGX.cmake")
INCLUDE ("$ENV{FPC_PATH}/common/crypto/pdo/common/CMakeVariables.txt")

# Put test artifacts under /tests subdirectory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

##################################################################################################
## Verify Evidence test app:
## this application tests evidence from input files
##################################################################################################

SET(VERIFY_EVIDENCE_APP verify_evidence_app)
PROJECT(${VERIFY_EVIDENCE_APP} CXX)

ADD_EXECUTABLE(${VERIFY_EVIDENCE_APP}
    main.cpp
    test.cpp
    "$ENV{FPC_PATH}/common/base64/base64.cpp"
    )

TARGET_INCLUDE_DIRECTORIES(${VERIFY_EVIDENCE_APP} PRIVATE "$ENV{FPC_PATH}/common")
TARGET_INCLUDE_DIRECTORIES(${VERIFY_EVIDENCE_APP} PRIVATE "$ENV{FPC_PATH}/common/base64")
TARGET_INCLUDE_DIRECTORIES(${VERIFY_EVIDENCE_APP} PRIVATE "$ENV{FPC_PATH}/common/crypto/attestation-api/attestation")
TARGET_INCLUDE_DIRECTORIES(${VERIFY_EVIDENCE_APP} PRIVATE "$ENV{FPC_PATH}/common/crypto/attestation-api/evidence")

TARGET_COMPILE_OPTIONS(${VERIFY_EVIDENCE_APP} PRIVATE ${COMMON_CXX_FLAGS} "-DPDO_DEBUG_BUILD=1 -D_UNTRUSTED_")

# Link the untrusted test application against the untrusted library and openssl
TARGET_LINK_LIBRARIES(${VERIFY_EVIDENCE_APP}
    "-L$ENV{FPC_PATH}/common/crypto/_build"
    "-Wl,--start-group" ${OPENSSL_LDFLAGS} "-l${U_CRYPTO_ADAPT_LIB_NAME}" "-l${U_CRYPTO_LIB_NAME}" "-Wl,--end-group"
    )

###################################################################################################
## Attestation app:
## this application performs a sim/hw attestation from input files and writes output on file
###################################################################################################

ADD_SUBDIRECTORY(trusted/enclave)

SET(GET_ATTESTATION_APP get_attestation_app)
PROJECT(${GET_ATTESTATION_APP} CXX C)

# ENCLAVE_EDL is defined in the enclave subdirectory
SGX_EDGE_UNTRUSTED(${ENCLAVE_EDL} ENCLAVE_EDGE_SOURCES)

ADD_EXECUTABLE(${GET_ATTESTATION_APP} trusted/app/main.cpp ${ENCLAVE_EDGE_SOURCES})

#TARGET_COMPILE_DEFINITIONS(${GET_ATTESTATION_APP} PRIVATE ${IAS_CA_CERT_REQUIRED_FLAGS})

# Make sure the enclave builds before the test app that links it
ADD_DEPENDENCIES(${GET_ATTESTATION_APP} test_enclave)

TARGET_LINK_LIBRARIES(${GET_ATTESTATION_APP} -Wl,-L,${SGX_SDK}/lib64)
TARGET_LINK_LIBRARIES(${GET_ATTESTATION_APP} -Wl,-L,${SGX_SSL}/lib64)
TARGET_LINK_LIBRARIES(${GET_ATTESTATION_APP} ${U_CRYPTO_ADAPT_LIB_NAME})
TARGET_LINK_LIBRARIES(${GET_ATTESTATION_APP} ${URTS_LIBRARY_NAME} ${AE_SERVICE_LIBRARY_NAME} sgx_usgxssl ${SGX_EPID_LIB})

TARGET_INCLUDE_DIRECTORIES(${GET_ATTESTATION_APP} PUBLIC $ENV{FPC_PATH}/common)

ADD_CUSTOM_COMMAND(
    TARGET ${GET_ATTESTATION_APP}
    PRE_BUILD
    COMMAND cp test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND cp trusted/enclave/test_enclave.config.xml ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND mkdir -p ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../scripts && cp ../scripts/* ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../scripts
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

# Register this application as a test
add_test(
    NAME "attestation_test_script"
    COMMAND bash ./test.sh
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

#################################################################################
## Run the tests automatically after building
#################################################################################
IF(DEFINED TEST_TARGET)
    ADD_DEPENDENCIES(${TEST_TARGET} ${VERIFY_EVIDENCE_APP})
    ADD_DEPENDENCIES(${TEST_TARGET} ${GET_ATTESTATION_APP})
endif()
