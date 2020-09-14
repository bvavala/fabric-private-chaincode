/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sgx_urts.h"
#include "sgx_error.h"
#include "sgx_eid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_enclave_u.h"

#include "error.h"
#include "logging.h"

#define ENCLAVE_FILENAME "test_enclave.signed.so"
#define INIT_DATA_INPUT "init_attestation_input.txt"
#define GET_ATTESTATION_OUTPUT "get_attestation_output.txt"
#define STATEMENT "1234567890"

int main()
{
    sgx_launch_token_t token = { 0 };
    int updated = 0;
    sgx_enclave_id_t global_eid = 0;
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    uint8_t attestation[4096];
    uint32_t attestation_length = 0;
    int b;
    char params[2048];
    FILE* fpi;
    FILE *fpo;

    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS)
    {
        puts("error creating enclave");
        exit(-1);
    }


    fpi = fopen(INIT_DATA_INPUT, "r+");
    COND2LOGERR(fpi == NULL, "can't open " INIT_DATA_INPUT "\n");
    COND2LOGERR(params != fgets(params, 2048, fpi), "error fgets");
    fclose(fpi);

    init_att(global_eid, &b, (uint8_t*)params, strlen(params));
    COND2LOGERR(!b, "init_attestation failed");

    get_att(global_eid, &b, (uint8_t*)STATEMENT, strlen(STATEMENT), attestation, 4096, &attestation_length);
    COND2LOGERR(!b, "get_attestation failed");

    fpo = fopen(GET_ATTESTATION_OUTPUT, "w+");
    COND2LOGERR(fpo == NULL, "can't open " GET_ATTESTATION_OUTPUT "\n");
    fwrite(attestation, sizeof(uint8_t), attestation_length, fpo);
    fclose(fpo);

    sgx_destroy_enclave(global_eid);
    return 0;

err:
    sgx_destroy_enclave(global_eid);
    return -1;
}

void ocall_print_string(const char* str)
{
    printf("%s", str);
}
