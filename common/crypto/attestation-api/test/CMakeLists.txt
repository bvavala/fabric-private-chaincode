# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE ("$ENV{FPC_PATH}/cmake/ConfigSGX.cmake")
INCLUDE ("$ENV{FPC_PATH}/common/crypto/pdo/common/CMakeVariables.txt")

# Put test artifacts under /tests subdirectory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

################################################################################
# Untrusted Test Application
################################################################################

SET(ATTESTATION_TEST_NAME attestation-test)
PROJECT(${ATTESTATION_TEST_NAME} CXX)

ADD_EXECUTABLE(${ATTESTATION_TEST_NAME}
    main.cpp
    test.cpp
    "$ENV{FPC_PATH}/common/base64/base64.cpp"
    )

TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE "$ENV{FPC_PATH}/common")
TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE "$ENV{FPC_PATH}/common/base64")
TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE "$ENV{FPC_PATH}/common/crypto/attestation-api/evidence")

TARGET_COMPILE_OPTIONS(${ATTESTATION_TEST_NAME} PRIVATE ${COMMON_CXX_FLAGS})

# Link the untrusted test application against the untrusted library and openssl
TARGET_LINK_LIBRARIES(${ATTESTATION_TEST_NAME}
    "-L$ENV{FPC_PATH}/common/crypto/_build"
    "-Wl,--start-group" ${OPENSSL_LDFLAGS} "-l${U_CRYPTO_ADAPT_LIB_NAME}" "-l${U_CRYPTO_LIB_NAME}" "-Wl,--end-group"
    )

# Register this application as a test
add_test(
	NAME ${ATTESTATION_TEST_NAME}
    COMMAND env LD_LIBRARY_PATH=${OPENSSL_LIBRARY_DIRS}:${LD_LIBRARY_PATH} ./${ATTESTATION_TEST_NAME}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

#################################################################################
## Trusted Test Application
#################################################################################

ADD_SUBDIRECTORY(trusted/enclave)

SET(TRUSTED_TEST_NAME get_attestation)
PROJECT(${TRUSTED_TEST_NAME} CXX C)

# ENCLAVE_EDL is defined in the enclave subdirectory
SGX_EDGE_UNTRUSTED(${ENCLAVE_EDL} ENCLAVE_EDGE_SOURCES)

ADD_EXECUTABLE(${TRUSTED_TEST_NAME} trusted/app/main.cpp ${ENCLAVE_EDGE_SOURCES})

#TARGET_COMPILE_DEFINITIONS(${TRUSTED_TEST_NAME} PRIVATE ${IAS_CA_CERT_REQUIRED_FLAGS})

# Make sure the enclave builds before the test app that links it
ADD_DEPENDENCIES(${TRUSTED_TEST_NAME} test_enclave)

TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} -Wl,-L,${SGX_SDK}/lib64)
TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} -Wl,-L,${SGX_SSL}/lib64)
TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} ${U_CRYPTO_ADAPT_LIB_NAME})
TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} ${URTS_LIBRARY_NAME} ${AE_SERVICE_LIBRARY_NAME} sgx_usgxssl ${SGX_EPID_LIB})

TARGET_INCLUDE_DIRECTORIES(${TRUSTED_TEST_NAME} PUBLIC $ENV{FPC_PATH}/common)

ADD_CUSTOM_COMMAND(
    TARGET ${TRUSTED_TEST_NAME}
    PRE_BUILD
    COMMAND cp test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

# Register this application as a test
add_test(
    NAME "attestation_test_script"
    #COMMAND ./${TRUSTED_TEST_NAME}
    COMMAND ./test.sh
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )

#################################################################################
## Run the tests automatically after building
#################################################################################
IF(DEFINED TEST_TARGET)
    ADD_DEPENDENCIES(${TEST_TARGET} ${ATTESTATION_TEST_NAME})
    ADD_DEPENDENCIES(${TEST_TARGET} ${TRUSTED_TEST_NAME})
endif()
