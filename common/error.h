/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "logging.h"

#define COND2ERR(b) \
    if(b) \
    { \
        LOG_ERROR("error at %s-%d", __FILE__, __LINE__); \
        goto err; \
    }

