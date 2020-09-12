# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

INCLUDE (../CMakeVariables.txt)

# Put test artifacts under /tests subdirectory
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/test)

#IF("$ENV{SGX_MODE}" STREQUAL "HW")
#    SET(IAS_CA_CERT_REQUIRED_FLAGS "-DIAS_CA_CERT_REQUIRED")
#ENDIF()

message("FPC path: $ENV{FPC_PATH}")

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
TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE "$ENV{FPC_PATH}/common/crypto/evidence")
#TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PUBLIC ${SGX_SDK}/include)
#TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE ${PDO_TOP_DIR}/common)
#TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE ${PDO_TOP_DIR}/common/crypto)
#TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE ${PDO_TOP_DIR}/common/tests/crypto)
#TARGET_INCLUDE_DIRECTORIES(${ATTESTATION_TEST_NAME} PRIVATE ${PDO_TOP_DIR}/common/packages/base64)

TARGET_COMPILE_OPTIONS(${ATTESTATION_TEST_NAME} PRIVATE ${COMMON_CXX_FLAGS})

## critical flag for conditional compile in test crypto sources
#TARGET_COMPILE_DEFINITIONS(${ATTESTATION_TEST_NAME} PRIVATE "-D_UNTRUSTED_=1")

#TARGET_COMPILE_DEFINITIONS(${ATTESTATION_TEST_NAME} PRIVATE ${IAS_CA_CERT_REQUIRED_FLAGS})

# Link the untrusted test application against the untrusted library and openssl
TARGET_LINK_LIBRARIES(${ATTESTATION_TEST_NAME}
    "-L$ENV{FPC_PATH}/common/crypto/_build"
    "-Wl,--start-group" ${OPENSSL_LDFLAGS} "-l${U_CRYPTO_ADAPT_LIB_NAME}" "-l${U_CRYPTO_LIB_NAME}" "-Wl,--end-group"
    )

# Register this application as a test
add_test(
	NAME ${ATTESTATION_TEST_NAME}
    COMMAND env LD_LIBRARY_PATH=${OPENSSL_LIBRARY_DIRS}:${LD_LIBRARY_PATH} ./${ATTESTATION_TEST_NAME}
	WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
)

#################################################################################
## Trusted Test Application
#################################################################################
#
#if(NOT UNTRUSTED_ONLY)
#	ADD_SUBDIRECTORY(trusted/enclave)
#
#	SET(TRUSTED_TEST_NAME ttest)
#	PROJECT(${TRUSTED_TEST_NAME} C CXX)
#
#	# ENCLAVE_EDL is defined in the enclave subdirectory
#	SGX_EDGE_UNTRUSTED(${ENCLAVE_EDL} ENCLAVE_EDGE_SOURCES)
#
#	ADD_EXECUTABLE(${TRUSTED_TEST_NAME} trusted/app/TestApp.cpp ${ENCLAVE_EDGE_SOURCES})
#
#    TARGET_COMPILE_DEFINITIONS(${TRUSTED_TEST_NAME} PRIVATE ${IAS_CA_CERT_REQUIRED_FLAGS})
#
#	# Make sure the enclave builds before the test app that links it
#	ADD_DEPENDENCIES(${TRUSTED_TEST_NAME} TestEnclave)
#
#	TARGET_INCLUDE_DIRECTORIES(${TRUSTED_TEST_NAME} PUBLIC ${SGX_SDK}/include)
#	TARGET_INCLUDE_DIRECTORIES(${TRUSTED_TEST_NAME} PRIVATE ${PDO_TOP_DIR}/common)
#
#	TARGET_COMPILE_OPTIONS(${TRUSTED_TEST_NAME} PRIVATE ${COMMON_CXX_FLAGS})
#
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} -Wl,-L,${SGX_SDK}/lib64)
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} -Wl,-L,${SGX_SSL}/lib64)
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} -Wl,-L,${SGX_SSL}/lib64/release)
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} ${URTS_LIBRARY_NAME} ${AE_SERVICE_LIBRARY_NAME} sgx_usgxssl)
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} pthread)
#	TARGET_LINK_LIBRARIES(${TRUSTED_TEST_NAME} ${UNTRUSTED_LIB_NAME})
#
#	# Register this application as a test
#	add_test(
#		NAME ${TRUSTED_TEST_NAME}
#		COMMAND ./${TRUSTED_TEST_NAME}
#		WORKING_DIRECTORY ${TESTS_OUTPUT_DIR}
#	)
#endif()

#################################################################################
## Run the tests automatically after building
#################################################################################
IF(DEFINED TEST_TARGET)
    ADD_DEPENDENCIES(${TEST_TARGET} ${ATTESTATION_TEST_NAME})
    #    if(NOT UNTRUSTED_ONLY)
    #    ADD_DEPENDENCIES(${TEST_TARGET} ${TRUSTED_TEST_NAME})
    #endif()
endif()
