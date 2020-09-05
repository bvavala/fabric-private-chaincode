/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "error.h"
#include "logging.h"
#include "parson.h"
#include "sgx_quote.h"
#include <string>
#include "pdo/common/crypto/crypto.h"
#include "types.h"
#include "enclave_t.h"
#include "sgx_utils.h"
#include "base64.h"

typedef struct {
    bool initialized;
    sgx_spid_t spid;
    ByteArray sig_rl;
    std::string attestation_type;
    uint32_t sign_type;
} attestation_state_t;

attestation_state_t g_attestation_state = {0};

bool init_attestation(
    uint8_t* params,
    uint32_t params_length)
{
    std::string params_string((char*)params, params_length);
    JSON_Value* root = json_parse_string(params_string.c_str());
    COND2LOGERR(root == NULL, "cannot parse attestation params");

    {   //set attestation type
        const char* p;
        p = json_object_get_string(json_object(root), "attestation_type");
        COND2LOGERR(p == NULL, "no attestation type provided");
        g_attestation_state.attestation_type.assign(p, p + strlen(p));

        if(g_attestation_state.attestation_type.compare("simulated") == 0)
        {
            //terminate init successfully
            goto init_success;
        }

        //check other types
        g_attestation_state.sign_type = -1;
        if(g_attestation_state.attestation_type.compare("epid-linkable") == 0)
           g_attestation_state.sign_type = SGX_LINKABLE_SIGNATURE;

        if(g_attestation_state.attestation_type.compare("epid-unlinkable") == 0)
           g_attestation_state.sign_type = SGX_UNLINKABLE_SIGNATURE;

        COND2LOGERR(g_attestation_state.sign_type == -1, "wrong attestation type");
    }

    {   //set SPID
        std::string hex_spid;
        const char* p;
        p = json_object_get_string(json_object(root), "hex_spid");
        COND2LOGERR(p == NULL, "no spid provided");

        hex_spid.assign(p);
        COND2LOGERR(hex_spid.length() != sizeof(sgx_spid_t) * 2, "wrong spid length");
        // translate spid to binary
        for (unsigned int i = 0; i < hex_spid.length(); i += 2)
        {
            std::string byteString = hex_spid.substr(i, 2);
            long int li_byte = strtol(byteString.c_str(), NULL, 16);
            g_attestation_state.spid.id[i / 2] = (char)li_byte;
        }
    }

    {   //set sig_rl
        const char* p;
        p = json_object_get_string(json_object(root), "sig_rl");
        COND2LOGERR(p == NULL, "no sig_rl provided");
        g_attestation_state.sig_rl.assign(p, p + strlen(p));
    }

init_success:
    g_attestation_state.initialized = true;
    return true;

err:
    return false;
}

bool get_attestation(
    uint8_t* statement,
    uint32_t statement_length,
    uint8_t* attestation,
    uint32_t attestation_max_length,
    uint32_t* attestation_length)
{
    sgx_report_t report;
    sgx_report_data_t report_data = {0};
    sgx_target_info_t qe_target_info = {0};
    sgx_epid_group_id_t egid = {0};
    std::string b64attestation;
    int ret;

    COND2ERR(statement == NULL);
    COND2ERR(attestation == NULL);
    COND2ERR(attestation_length == NULL || attestation_max_length == 0);

    if(g_attestation_state.attestation_type.compare("simulated") == 0)
    {
        attestation[0] = '0';
        *attestation_length = 1;
    }
    else
    {
        ocall_init_quote(
                (uint8_t*)&qe_target_info, sizeof(qe_target_info), (uint8_t*)&egid, sizeof(egid));

        ByteArray fixed_size_statement(statement, statement + statement_length);
        ByteArray rd = pdo::crypto::ComputeMessageHash(fixed_size_statement);
        // ComputeMessageHash uses sha256
        COND2LOGERR(rd.size() > sizeof(sgx_report_data_t), "report data too long");
        memcpy(&report_data, rd.data(), rd.size());

        ret = sgx_create_report(&qe_target_info, &report_data, &report);
        COND2LOGERR(SGX_SUCCESS != ret, "error creating report");

        ocall_get_quote(
                (uint8_t*)&g_attestation_state.spid,
                (uint32_t)sizeof(sgx_spid_t),
                g_attestation_state.sig_rl.data(),
                g_attestation_state.sig_rl.size(),
                g_attestation_state.sign_type,
                (uint8_t*)&report,
                sizeof(report),
                attestation,
                attestation_max_length,
                attestation_length);
        COND2LOGERR(*attestation_length == 0, "error get quote");

        //convert to base64
        b64attestation = base64_encode((const unsigned char*)attestation, *attestation_length);
        LOG_DEBUG("b64 attestation: %s", b64attestation.c_str());
        COND2LOGERR(b64attestation.length() > attestation_max_length, "not enough space for b64 conversion");
        memcpy(attestation, b64attestation.c_str(), b64attestation.length());
        *attestation_length = b64attestation.length();
    }
    return true;
err:
    return false;
}
