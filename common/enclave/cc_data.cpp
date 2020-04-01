/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_data.h"
#include <pb_encode.h>
#include "protos/chaincode_enclave/credentials.pb.h"
#include "error.h"
#include "report_data.h"

cc_data g_cc_data;

bool cc_data::generate()
{
    return kr.generate();
}

bool cc_data::to_public_proto(const std::string& hex_spid, uint8_t* buf, size_t buf_size, size_t* out_size)
{
    report_data rd(kr);
    pb_ostream_t ostream = pb_ostream_from_buffer(buf, buf_size);
    {
        // verb
        const unsigned char *verb = (const unsigned char*)"CREATE";
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_verb_tag));
        COND2ERR(!pb_encode_string(&ostream, verb, strlen((const char*)verb)+1));
    }
    {
        // creds
        size_t size;
        const void *p = rd.get_proto_creds(&size);
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_reportDataFields_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)p, size));
    }
    {
        // attestation
        std::string b64_quote;
        COND2ERR(!rd.b64_quote(hex_spid, b64_quote));
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_enclaveAttestation_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)b64_quote.c_str(), b64_quote.length()));
    }
    *out_size = ostream.bytes_written;
    return true;

err:
    return false;
}

bool cc_data::to_private_proto(uint8_t* buf, size_t buf_size, size_t* out_size)
{
    //TODO
    return false;
}

bool from_public_from_proto(uint8_t* buf, size_t buf_size)
{
    //TODO
    return false;
}

