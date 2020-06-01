/*
 * Copyright 2019 Intel Corporation
 * Copyright IBM Corp. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sgxcclib.h"
#include "common-sgxcclib.h"  //CHECK_SGX_ERROR_AND_RETURN_ON_ERROR macro
#include "enclave_u.h"
#include "sgx_attestation_type.h"

#include <stdbool.h>
#include <string.h>

#include "sgx_quote.h"

#include "sgx_urts.h"

extern void LOG_DEBUG(const char* fmt, ...);
extern void LOG_ERROR(const char* fmt, ...);

// - creator access
extern void get_creator_name(
    const char* msp_id, uint32_t max_msp_id_len, const char* dn, uint32_t max_dn_len, void* ctx);

// - channel id
extern void get_channel_id(const char* channel_id, uint32_t max_channel_id_len, void* ctx);

// - msp id
extern void get_msp_id(const char* msp_id, uint32_t max_msp_id_len, void* ctx);

// - for accessing ledger kvs
extern void get_state(const char* key,
    uint8_t* val,
    uint32_t max_val_len,
    uint32_t* val_len,
    cmac_t* cmac,
    void* ctx);
extern void get_state_by_partial_composite_key(const char* comp_key,
    uint8_t* values,
    uint32_t max_len,
    uint32_t* values_len,
    cmac_t* cmac,
    void* ctx);
extern void put_state(const char* key, uint8_t* val, uint32_t val_len, void* ctx);

int sgxcc_bind(enclave_id_t eid, report_t* report, ec256_public_t* pubkey)
{
    int enclave_ret = SGX_ERROR_UNEXPECTED;
    int ret = ecall_bind_tlcc(eid, &enclave_ret, (sgx_report_t*)report, (uint8_t*)pubkey);
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(ret)
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(enclave_ret)
    return SGX_SUCCESS;
}

int sgxcc_invoke(enclave_id_t eid,
    const char* encoded_args,
    const char* pk,
    uint8_t* response,
    uint32_t response_len_in,
    uint32_t* response_len_out,
    ec256_signature_t* signature,
    void* ctx)
{
    int enclave_ret = SGX_ERROR_UNEXPECTED;
    int ret = ecall_cc_invoke(eid, &enclave_ret,
        encoded_args,  // args  (encoded and potentially encrypted)
        pk,            // client pk used for args encryption, if null no encryption used
        response, response_len_in, response_len_out,  // response
        (sgx_ec256_signature_t*)signature,            // signature
        ctx);                                         // context for callback
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(ret)
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(enclave_ret)
    return SGX_SUCCESS;
}

/* OCall functions */
void ocall_get_creator_name(
    char* msp_id, uint32_t max_msp_id_len, char* dn, uint32_t max_dn_len, void* ctx)
{
    get_creator_name(msp_id, max_msp_id_len, dn, max_dn_len, ctx);
}

void ocall_get_channel_id(char* channel_id, uint32_t max_channel_id_len, void* ctx)
{
    get_channel_id(channel_id, max_channel_id_len, ctx);
}

void ocall_get_msp_id(char* msp_id, uint32_t max_msp_id_len, void* ctx)
{
    get_msp_id(msp_id, max_msp_id_len, ctx);
}

void ocall_get_state(const char* key,
    uint8_t* val,
    uint32_t max_val_len,
    uint32_t* val_len,
    sgx_cmac_128bit_tag_t* cmac,
    void* ctx)
{
    get_state(key, val, max_val_len, val_len, (cmac_t*)cmac, ctx);
}

void ocall_put_state(const char* key, uint8_t* val, uint32_t val_len, void* ctx)
{
    put_state(key, val, val_len, ctx);
}

void ocall_get_state_by_partial_composite_key(const char* key,
    uint8_t* bids_bytes,
    uint32_t max_len,
    uint32_t* bids_bytes_len,
    sgx_cmac_128bit_tag_t* cmac,
    void* ctx)
{
    get_state_by_partial_composite_key(
        key, bids_bytes, max_len, bids_bytes_len, (cmac_t*)cmac, ctx);
}

void ocall_print_string(const char* str)
{
    golog(str);
}

void ocall_init_quote(uint8_t* target, uint32_t target_len, uint8_t* egid, uint32_t egid_len)
{
    int ret = sgx_init_quote(target, egid);
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(ret)
}

void ocall_get_quote(uint8_t* spid,
    uint32_t spid_len,
    uint8_t* report,
    uint32_t report_len,
    uint8_t* quote,
    uint32_t max_quote_len,
    uint32_t* actual_quote_len)
{
    int ret;
    uint32_t required_quote_size = 0;
    ret = sgxcc_get_quote_size(NULL, 0, &required_quote_size);
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(ret)
    // assume enough quote size
    ret = sgx_get_quote(report, SGX_QUOTE_SIGN_TYPE,
        (sgx_spid_t*)spid,  // spid
        NULL,               // nonce
        NULL,               // sig_rl
        0,                  // sig_rl_size
        NULL,               // p_qe_report
        (sgx_quote_t*)quote, required_quote_size);
    CHECK_SGX_ERROR_AND_RETURN_ON_ERROR(ret)
    *actual_quote_len = required_quote_size;
}
