/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "attestation.h"
#include <pb_encode.h>
#include "base64.h"
#include "enclave_t.h"
#include "error.h"
#include "key_ring.h"
#include "logging.h"
#include "protos/credentials.pb.h"
#include "sgx_quote.h"
#include "sgx_report.h"
#include "sgx_utils.h"
#include "utils.h"

attestation::attestation(const key_ring& kr, const cc_parameters& ccp)
{
    // populated attested data proto
    pb_ostream_t ostream = pb_ostream_from_buffer(attested_data_proto, attested_data_proto_size);
    {
        size_t proto_size = (1 << 10);
        size_t actual_proto_size;
        uint8_t proto[proto_size];
        COND2ERR(!kr.to_public_proto(proto, proto_size, &actual_proto_size));
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, attestedData_enclavePublicKeys_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)proto, actual_proto_size));
    }
    {
        size_t proto_size = (1 << 10);
        size_t actual_proto_size;
        uint8_t proto[proto_size];
        COND2ERR(!ccp.to_proto(proto, proto_size, &actual_proto_size));
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, attestedData_ccParameters_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)proto, actual_proto_size));
    }

    // get size of serialized attested data
    actual_attested_data_proto_size = ostream.bytes_written;

    {
        // dump for debug
        char* s = bytes_to_hexstring(attested_data_proto, actual_attested_data_proto_size);
        LOG_DEBUG(
            "attested data proto (binary len %u), hex: %s", actual_attested_data_proto_size, s);
    }

    // compute report data for attestation
    sgx_sha256_msg(
        attested_data_proto, actual_attested_data_proto_size, (sgx_sha256_hash_t*)&report_data_);
    {
        // dump for debug
        char* s = bytes_to_hexstring((uint8_t*)&report_data_, sizeof(sgx_report_data_t));
        LOG_DEBUG("reportdata: %s", s);
    }
    return;

err:
    actual_attested_data_proto_size = 0;
    LOG_ERROR("report data failed");
}

const void* attestation::get_attested_data_proto(size_t* size) const
{
    *size = actual_attested_data_proto_size;
    return attested_data_proto;
}

bool attestation::b64_quote(const std::string& hex_spid, std::string& b64_quote) const
{
    sgx_report_t report;
    sgx_target_info_t qe_target_info = {0};
    sgx_epid_group_id_t egid = {0};
    sgx_spid_t spid;
    uint8_t quote[2048];
    uint32_t quote_size = 0;

    COND2ERR(hex_spid.length() != sizeof(sgx_spid_t) * 2);

    // translate spid to binary
    for (unsigned int i = 0; i < hex_spid.length(); i += 2)
    {
        std::string byteString = hex_spid.substr(i, 2);
        long int li_byte = strtol(byteString.c_str(), NULL, 16);
        COND2ERR(li_byte < 0 || li_byte > 0xFF);
        char byte = (char)li_byte;
        spid.id[i / 2] = byte;
    }

    COND2ERR(SGX_SUCCESS != ocall_init_quote((uint8_t*)&qe_target_info, sizeof(qe_target_info),
                                (uint8_t*)&egid, sizeof(egid)));
    COND2ERR(SGX_SUCCESS != sgx_create_report(&qe_target_info, &report_data_, &report));
    COND2ERR(SGX_SUCCESS != ocall_get_quote((uint8_t*)&spid, (uint32_t)sizeof(sgx_spid_t),
                                (uint8_t*)&report, (uint32_t)sizeof(sgx_report_t), (uint8_t*)&quote,
                                2048, &quote_size));

    b64_quote = base64_encode((const unsigned char*)quote, quote_size);

    return true;

err:
    return false;
}
