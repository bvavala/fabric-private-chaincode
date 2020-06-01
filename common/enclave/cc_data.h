/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "cc_parameters.h"
#include "key_ring.h"
#include "shim.h"

class cc_data
{
private:
    key_ring kr_;
    cc_parameters cc_parameters_;

public:
    bool generate(shim_ctx_ptr_t& ctx);
    bool to_public_proto(
        const std::string& hex_spid, uint8_t* buf, size_t buf_size, size_t* out_size);
    bool to_private_proto(uint8_t* buf, size_t buf_size, size_t* out_size);
    bool from_private_proto(uint8_t* buf, size_t buf_size);
};

extern cc_data* g_cc_data;
