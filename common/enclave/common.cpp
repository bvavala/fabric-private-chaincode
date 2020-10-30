/*
 * Copyright IBM Corp. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "enclave_t.h"

#include "base64.h"
#include "logging.h"
#include "utils.h"

#include <assert.h>
#include <string>

#include "sgx_utils.h"

#include "attestation-api/attestation/attestation.h"
#include "error.h"

#include <pb_decode.h>
#include "protos/fpc/attestation.pb.h"
#include "protos/fpc/fpc.pb.h"

#include "cc_data.h"

// enclave sk and pk (both are little endian) used for out signatures
sgx_ec256_private_t enclave_sk = {0};
sgx_ec256_public_t enclave_pk = {0};

// creates new identity if not exists
int ecall_init(const uint8_t* attestation_parameters,
    uint32_t ap_size,
    const uint8_t* cc_parameters,
    uint32_t ccp_size,
    const uint8_t* host_parameters,
    uint32_t hp_size,
    uint8_t* credentials,
    uint32_t credentials_max_size,
    uint32_t* credentials_size)
{
    bool b;

    // create new pub/prv key pair
    sgx_ecc_state_handle_t ecc_handle = NULL;
    sgx_status_t sgx_ret = sgx_ecc256_open_context(&ecc_handle);
    if (sgx_ret != SGX_SUCCESS)
    {
        LOG_DEBUG("Enclave: sgx_ecc256_open_context: %d", sgx_ret);
        return sgx_ret;
    }

    // create pub and private signature key
    sgx_ret = sgx_ecc256_create_key_pair(&enclave_sk, &enclave_pk, ecc_handle);
    if (sgx_ret != SGX_SUCCESS)
    {
        LOG_DEBUG("Enclave: sgx_ecc256_create_key_pair: %d", sgx_ret);
        return sgx_ret;
    }
    sgx_ecc256_close_context(ecc_handle);

    std::string base64_pk =
        base64_encode((const unsigned char*)&enclave_pk, sizeof(sgx_ec256_public_t));
    LOG_DEBUG("Enc: Enclave pk (little endian): %s", base64_pk.c_str());

    LOG_DEBUG("Enc: Identity generated!");

    {  // TODO: this block is here temporarily, only meant to test the attestation api
        char params[] = "{\"attestation_type\":\"simulated\"}";
        // char params[] = "{\"attestation_type\":\"simulated\"}";
        bool bb = init_attestation((uint8_t*)params, strlen(params));
        COND2LOGERR(bb == false, "error init attestation");
        LOG_DEBUG("init attestation successful");

        char statement[] = "1234567890";
        uint8_t attestation[2048];
        uint32_t attestation_length = 0;
        bool b = get_attestation(
            (uint8_t*)statement, strlen(statement), attestation, 2048, &attestation_length);
        COND2LOGERR(b == false, "error getting attestation");
        LOG_DEBUG("get attestation success, size %d", attestation_length);
    }

    // debug attestation_parameters (can be removed)
    if (attestation_parameters != NULL && ap_size > 0)
    {
        pb_istream_t is, sis;
        pb_wire_type_t wt;
        uint32_t tag;
        bool eof;

        is = pb_istream_from_buffer(attestation_parameters, ap_size);
        COND2LOGERR(!pb_decode_tag(&is, &wt, &tag, &eof), "cannot decode tag");
        COND2LOGERR(wt != PB_WT_STRING, "unexpected type");
        COND2LOGERR(tag != attestation_AttestationParameters_parameters_tag, "unexpected tag");

        COND2LOGERR(!pb_make_string_substream(&is, &sis), "substream error");

        {
            uint32_t buffer_len = sis.bytes_left + 1;
            char buffer[buffer_len];
            COND2LOGERR(!pb_read(&sis, (pb_byte_t*)buffer, sis.bytes_left), "cannot read field");
            buffer[buffer_len - 1] = '\0';
            LOG_DEBUG("attestation parameters field: %s", buffer);

            std::string dec = base64_decode(buffer);
            LOG_DEBUG("decoded attestation parameters: %s", dec.c_str());
        }

        pb_close_string_substream(&is, &sis);
    }

    // debug cc_parameters (can be removed)
    if (cc_parameters != NULL && ccp_size > 0)
    {
        pb_istream_t is, sis;
        pb_wire_type_t wt;
        uint32_t tag;
        bool eof;

        is = pb_istream_from_buffer(cc_parameters, ccp_size);
        COND2LOGERR(!pb_decode_tag(&is, &wt, &tag, &eof), "cannot decode tag");
        COND2LOGERR(wt != PB_WT_STRING, "unexpected type");
        COND2LOGERR(tag != fpc_CC_Parameters_chaincode_id_tag, "unexpected tag");

        COND2LOGERR(!pb_make_string_substream(&is, &sis), "substream error");

        {
            uint32_t buffer_len = sis.bytes_left + 1;
            char buffer[buffer_len];
            COND2LOGERR(!pb_read(&sis, (pb_byte_t*)buffer, sis.bytes_left), "cannot read field");
            buffer[buffer_len - 1] = '\0';
            LOG_DEBUG("cc id field: %s", buffer);
        }

        pb_close_string_substream(&is, &sis);
    }

    // debug host parameters (can be removed)
    if (host_parameters != NULL && hp_size > 0)
    {
        std::string hp((char*)host_parameters, hp_size);
        LOG_DEBUG("hp_size %d", hp_size);
        LOG_DEBUG("hp: %s", hp.c_str());
    }

    // NOTE:
    // cc_data is a global pointer, meant to reference a global variable of cc_data type.
    // If g_cc_data is implemented as a simple global variable, cgo seems to crash (on enclave
    // destroy). This seems due to constructor/destructor issues -- if the variable is declared
    // in a function, it works. For this reason, we allocate it here dynamically.
    // TODO: free this memory when necessary.
    COND2LOGERR(g_cc_data != NULL, "cc data already created");

    g_cc_data = new cc_data;
    COND2LOGERR(g_cc_data == NULL, "error creating cc data object");

    b = g_cc_data->generate();
    COND2LOGERR(!b, "error generating cc data");

    // if a credential buffer was provided, then get credentials, else ignore
    if (credentials_max_size > 0)
    {
        b = g_cc_data->get_credentials(attestation_parameters, ap_size, cc_parameters, ccp_size,
            host_parameters, hp_size, credentials, credentials_max_size, credentials_size);
        COND2LOGERR(!b, "error getting credentials");
    }

    return SGX_SUCCESS;

err:
    return SGX_ERROR_UNEXPECTED;
}

// returns report (containing enclave pk hash) and enclave pk in big endian format
int ecall_create_report(
    const sgx_target_info_t* target, sgx_report_t* report_out, uint8_t* pubkey_out)
{
    sgx_report_t report;
    sgx_report_data_t report_data = {{0}};

    memset(&report, 0, sizeof(report));

    // transform enclave_pk to Big Endian before hashing
    uint8_t enclave_pk_be[sizeof(sgx_ec256_public_t)];
    memcpy(enclave_pk_be, &enclave_pk, sizeof(sgx_ec256_public_t));
    bytes_swap(enclave_pk_be, 32);
    bytes_swap(enclave_pk_be + 32, 32);

    // write H(enclave_pk) in report data
    assert(sizeof(report_data) >= sizeof(sgx_sha256_hash_t));
    sgx_sha256_msg(enclave_pk_be, sizeof(sgx_ec256_public_t), (sgx_sha256_hash_t*)&report_data);

    // copy enclave_pk_be outside
    memcpy(pubkey_out, enclave_pk_be, sizeof(sgx_ec256_public_t));

    // create the report
    sgx_status_t ret = sgx_create_report(target, &report_data, &report);
    if (ret != SGX_SUCCESS)
    {
        LOG_ERROR("Enclave: Error while creating report");
        return ret;
    }
    memcpy(report_out, &report, sizeof(sgx_report_t));

    LOG_DEBUG("Enc: Report generated!");
    return SGX_SUCCESS;
}

// returns enclave pk in Big Endian format
int ecall_get_pk(uint8_t* pubkey)
{
    // transform enclave_pk to Big Endian before hashing
    uint8_t enclave_pk_be[sizeof(sgx_ec256_public_t)];
    memcpy(enclave_pk_be, &enclave_pk, sizeof(sgx_ec256_public_t));
    bytes_swap(enclave_pk_be, 32);
    bytes_swap(enclave_pk_be + 32, 32);

    memcpy(pubkey, &enclave_pk_be, sizeof(sgx_ec256_public_t));

    LOG_DEBUG("Enc: Return enclave pk as Big Endian");
    return SGX_SUCCESS;
}
