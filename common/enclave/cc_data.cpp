/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// common deps
#include "cc_data.h"
#include "error.h"
#include "attestation.h"
#include "logging.h"

// protobuf deps
#include <pb_encode.h>
#include "protos/credentials.pb.h"

// shim deps
#include "shim.h"


// ecc enclave global variable -- allocated dynamically
cc_data* g_cc_data = NULL;

bool cc_data::generate(shim_ctx_ptr_t& ctx)
{
    {   // get channel id
        char channel_id[256];
        get_channel_id((char*)channel_id, sizeof(channel_id), ctx);
        cc_parameters_.channel_id_ = std::string(channel_id, sizeof(channel_id));
        LOG_DEBUG("in-enclave channel id: %s", cc_parameters_.channel_id_.c_str());
    }
    {   // get local msp id
        char msp_id[256];
        get_msp_id((char*)msp_id, sizeof(msp_id), ctx);
        cc_parameters_.msp_id_ = std::string(msp_id, sizeof(msp_id));
        LOG_DEBUG("in-enclave msp id: %s", cc_parameters_.msp_id_.c_str());
    }
    {   //get tlcc mrenclave
        //TODO
        cc_parameters_.tlcc_mrenclave_ = std::string("critical-tlcc-id");
        LOG_DEBUG("in-enclave tlcc mrenclave: %s", cc_parameters_.tlcc_mrenclave_);
    }
    {
        // chaincode id, version, sequence
        cc_parameters_.cc_id_ = std::string("fantasy");
        cc_parameters_.cc_version_ = std::string("music");
        cc_parameters_.cc_sequence_ = 678;
    }

    //generate cryptographic keys
    return kr_.generate();
}

bool cc_data::to_public_proto(const std::string& hex_spid, uint8_t* buf, size_t buf_size, size_t* out_size)
{
    // construct attestation class -- needed for attestation block and attested_data block
    attestation att(kr_, cc_parameters_);

    pb_ostream_t ostream = pb_ostream_from_buffer(buf, buf_size);
    {
        // verb
        const unsigned char *verb = (const unsigned char*)"CREATE";
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_verb_tag));
        COND2ERR(!pb_encode_string(&ostream, verb, strlen((const char*)verb)+1));
    }
    {
        // attested_data
        size_t size;
        const void *p = att.get_attested_data_proto(&size);
        COND2ERR(size == 0);
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, credentials_attestedData_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)p, size));
    }
    {
        // attestation
        std::string b64_quote;
        COND2ERR(!att.b64_quote(hex_spid, b64_quote));
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

