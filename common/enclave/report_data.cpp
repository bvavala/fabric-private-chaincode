/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "report_data.h"
#include "key_ring.h"
#include "sgx_report.h"
#include "sgx_quote.h"
#include "logging.h"
#include "error.h"
#include "base64.h"
#include "utils.h"
#include <pb_encode.h>
#include "protos/credentials.pb.h"
#include "enclave_t.h"
#include "sgx_report.h"
#include "sgx_quote.h"
#include "sgx_utils.h"

report_data::report_data(const key_ring& kr)
{
    // put keys in proto creds
    pb_ostream_t ostream = pb_ostream_from_buffer(proto_creds, proto_creds_size);
    {
        std::string b64;
        kr.to_b64(KT_SGX_RSA_PUBLIC, b64);
        //COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, reportDataFields_publicEncryptionKey_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)b64.c_str(), b64.length()));
    }
    actual_proto_creds_size = ostream.bytes_written;

    {
        char* s = bytes_to_hexstring(proto_creds, actual_proto_creds_size);
        LOG_DEBUG("proto creds (len %u: %s", actual_proto_creds_size, s);
    }

    sgx_sha256_msg(proto_creds, actual_proto_creds_size, (sgx_sha256_hash_t*)&report_data_);
    {
        char* s = bytes_to_hexstring((uint8_t*)&report_data_, sizeof(sgx_report_data_t));
        LOG_DEBUG("reportdata: %s", s);
    }
    return;

err:
    LOG_ERROR("report data failed");
}

const void* report_data::get_proto_creds(size_t* size) const
{
    *size = actual_proto_creds_size;
    return proto_creds;
}

bool report_data::b64_quote(const std::string& hex_spid, std::string& b64_quote) const
{
    sgx_report_t report;
    sgx_target_info_t qe_target_info = {0};
    sgx_epid_group_id_t egid = {0};
    sgx_spid_t spid;
    uint8_t quote[2048];
    uint32_t quote_size = 0; 

    COND2ERR(hex_spid.length() != sizeof(sgx_spid_t)*2);
    for (unsigned int i = 0; i < hex_spid.length(); i += 2)
    {
        std::string byteString = hex_spid.substr(i, 2);
        long int li_byte = strtol(byteString.c_str(), NULL, 16);
        COND2ERR(li_byte < 0 || li_byte > 0xFF);
        char byte = (char) li_byte;
        spid.id[i/2] = byte;
    }

    COND2ERR(SGX_SUCCESS != ocall_init_quote((uint8_t*)&qe_target_info, sizeof(qe_target_info), (uint8_t*)&egid, sizeof(egid)));
    COND2ERR(SGX_SUCCESS != sgx_create_report(&qe_target_info, &report_data_, &report));
    COND2ERR(SGX_SUCCESS != ocall_get_quote((uint8_t*)&spid, (uint32_t)sizeof(sgx_spid_t), (uint8_t*)&report, (uint32_t)sizeof(sgx_report_t), (uint8_t*)&quote, 2048, &quote_size));

    b64_quote = base64_encode((const unsigned char*)quote, quote_size);

    return true;

err:
    return false;
}

