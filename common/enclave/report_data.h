/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "key_ring.h"

class report_data
{
    private:
        sgx_report_data_t report_data_ = {{0}};
        const static size_t proto_creds_size = (1 << 12);
        size_t actual_proto_creds_size;
        uint8_t proto_creds[proto_creds_size];

    public:
        report_data(const key_ring& kr);
        const void* get_proto_creds(size_t* size) const;
        bool b64_quote(const std::string& hex_spid, std::string& b64_quote) const;
};

