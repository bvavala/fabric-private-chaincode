/*
 * Copyrighti 2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "base64.h"
#include "enclave_t.h"
#include "error.h"
#include "fpc-types.h"
#include "logging.h"
#include "sgx_quote.h"
#include "sgx_report.h"
#include "sgx_utils.h"
#include "shim.h"
#include "utils.h"

#include "cc_data.h"

#include <pb_encode.h>
#include "protos/credentials.pb.h"

extern sgx_ec256_public_t enclave_pk;

int ctl_invoke(
    uint8_t* response, uint32_t max_response_len, uint32_t* actual_response_len, shim_ctx_ptr_t ctx)
{
    LOG_DEBUG("ctl invoke");
    std::string func_name;
    std::vector<std::string> params;

    if (get_func_and_params(func_name, params, ctx) == -1)
    {
        LOG_ERROR("erro func params");
        return -1;
    }

    LOG_DEBUG("func %s , spid %s", func_name.c_str(), params[0].c_str());
    if (!func_name.compare("ctl_create"))
    {
        LOG_DEBUG("ctl_create");

        // NOTE:
        // cc_data is a global pointer, meant to reference a global variable of cc_data type.
        // If g_cc_data is implemented as a simple global variable, cgo seems to crash (on enclave
        // destroy). This seems due to constructor/destructor issues -- if the variable is declared
        // in a function, it works. For this reason, we allocate it here dynamically.
        // TODO: free this memory when necessary.
        g_cc_data = new cc_data;

        COND2ERR(!g_cc_data->generate(ctx));
        LOG_DEBUG("ccdata generate passed");
        COND2ERR(!g_cc_data->to_public_proto(
            params[0], response, max_response_len, (size_t*)actual_response_len));
        return 0;

        sgx_report_t report;
        sgx_report_data_t report_data = {{0}};

        sgx_target_info_t qe_target_info = {0};
        sgx_epid_group_id_t egid = {0};
        spid_t spid;
        int ret;

        ocall_init_quote(
            (uint8_t*)&qe_target_info, sizeof(qe_target_info), (uint8_t*)&egid, sizeof(egid));

        // expect 32char string for spid
        for (unsigned int i = 0; i < 32; i += 2)
        {
            std::string byteString = params[0].substr(i, 2);
            char byte = (char)strtol(byteString.c_str(), NULL, 16);
            LOG_DEBUG("b=%d %x", byte, byte);
            spid.id[i / 2] = byte;
        }

        memset(&report, 0, sizeof(report));

        // transform enclave_pk to Big Endian before hashing
        uint8_t enclave_pk_be[sizeof(sgx_ec256_public_t)];
        memcpy(enclave_pk_be, &enclave_pk, sizeof(sgx_ec256_public_t));
        bytes_swap(enclave_pk_be, 32);
        bytes_swap(enclave_pk_be + 32, 32);

        // write H(enclave_pk) in report data
        assert(sizeof(report_data) >= sizeof(sgx_sha256_hash_t));
        sgx_sha256_msg(enclave_pk_be, sizeof(sgx_ec256_public_t), (sgx_sha256_hash_t*)&report_data);
        // create the report
        LOG_DEBUG("create report");
        ret = sgx_create_report(&qe_target_info, &report_data, &report);
        if (ret != SGX_SUCCESS)
        {
            LOG_ERROR("errore creating report");
            return ret;
        }

        uint8_t quote[2048];
        uint32_t quote_size = 0;
        ocall_get_quote((uint8_t*)&spid, (uint32_t)sizeof(spid_t), (uint8_t*)&report,
            (uint32_t)sizeof(sgx_report_t), (uint8_t*)&quote, 2048, &quote_size);
        LOG_DEBUG("received quote size %u", quote_size);
        std::string b64quote = base64_encode((const unsigned char*)quote, quote_size);
        *actual_response_len = b64quote.length();
        LOG_DEBUG("b64 quote: %s", b64quote.c_str());

        // write response
        pb_ostream_t ostream = pb_ostream_from_buffer(response, max_response_len);
        const unsigned char* verb = (const unsigned char*)"CREATE";
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_verb_tag));
        COND2ERR(!pb_encode_string(&ostream, verb, strlen((const char*)verb) + 1));
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_enclaveAttestation_tag));
        COND2ERR(
            !pb_encode_string(&ostream, (const unsigned char*)b64quote.c_str(), b64quote.length()));

        *actual_response_len = ostream.bytes_written;
        LOG_DEBUG("ctl_create return %d", *actual_response_len);

        return 0;
    }
    LOG_ERROR("wrong function");

err:
    return -1;
}
