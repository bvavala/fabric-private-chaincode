/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "key_ring.h"
#include "cc_parameters.h"

class attestation
{
    private:
        sgx_report_data_t report_data_ = {{0}};
        const static size_t attested_data_proto_size = (1 << 12);
        size_t actual_attested_data_proto_size;
        uint8_t attested_data_proto[attested_data_proto_size];

    public:
        attestation(const key_ring& kr, const cc_parameters& ccp);
        const void* get_attested_data_proto(size_t* size) const;
        bool b64_quote(const std::string& hex_spid, std::string& b64_quote) const;
};

