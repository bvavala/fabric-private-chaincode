/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

class cc_parameters
{
public:
    std::string channel_id_;
    std::string msp_id_;
    std::string tlcc_mrenclave_;

    std::string cc_id_;
    std::string cc_version_;
    uint64_t cc_sequence_;

    bool to_proto(uint8_t*, size_t, size_t*) const;
};
