/*
 * Copyright 2020 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include "shim.h"
#include "logging.h"

#define CONTROL_API_PREFIX "ctl_"

bool is_ctl_invoke(shim_ctx_ptr_t ctx)
{
    std::string func_name;
    std::vector<std::string> params;

    LOG_DEBUG("check if ctl invoke");

    if(get_func_and_params(func_name, params, ctx) == -1)
    {
        LOG_DEBUG("Cannot parse func and params. This is not ctl invoke");
        return false;
    }

    bool b = (func_name.rfind(CONTROL_API_PREFIX, 0) == 0); // true if prefix is at position 0
    return b;
}

