/*
 * Copyrighti 2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

int ctl_invoke(uint8_t* response,
    uint32_t max_response_len,
    uint32_t* actual_response_len,
    shim_ctx_ptr_t ctx);

