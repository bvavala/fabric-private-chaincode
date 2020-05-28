/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_data.h"
#include <pb_encode.h>
#include "protos/credentials.pb.h"
#include "error.h"
#include "attestation.h"
#include "shim.h"
#include "logging.h"

bool cc_parameters::to_proto(uint8_t* buf, size_t buf_size, size_t* out_size) const
{
    pb_ostream_t ostream = pb_ostream_from_buffer(buf, buf_size);
    {
        // channel id
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, ccParameters_ccId_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)channel_id_.c_str(), channel_id_.length()+1));
    }
    {
        // msp id
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, ccParameters_mspId_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)msp_id_.c_str(), msp_id_.length()+1));
    }
    {
        // tlcc mrenclave
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, ccParameters_tlccMREnclave_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)tlcc_mrenclave_.c_str(), tlcc_mrenclave_.length()+1));
    }
    {
        // cc id
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, ccParameters_ccId_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)cc_id_.c_str(), cc_id_.length()+1));
    }
    {
        // cc version
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, ccParameters_ccVersion_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)cc_version_.c_str(), cc_version_.length()+1));
    }
    {   
        // cc sequence
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_64BIT, ccParameters_ccSequence_tag));
        COND2ERR(!pb_encode_varint(&ostream, cc_sequence_));
    }
    
    *out_size = ostream.bytes_written;
    LOG_DEBUG("cc_parameters proto size %d", *out_size);
    return true;

err:
    return false;
}

