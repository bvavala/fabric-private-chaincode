/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "error.h"
#include "logging.h"

int main()
{
    int r;

    LOG_INFO("\x1B[31m IF YOU READ THIS, IT'S AN ERROR - 1");  // this should not be displayed

    r = loggingf(
        "\x1B[31m IF YOU READ THIS, IT'S AN ERROR - 2");  // this fails, should not be displayed
    COND2ERR(r > 0);

    r = logging_set_callback(&puts);
    COND2ERR(r == 0);

    r = loggingf("HOPEFULLY YOU READ THIS! -3");  // this succeeds, should be displayed
    COND2ERR(r == 0);

    LOG_DEBUG("If you read this, log DEBUG is enabled");
    LOG_INFO("If you read this, log INFO is enabled");
    LOG_WARNING("If you read this, log WARNING is enabled");
    LOG_INFO("If you read this, log ERROR is enabled");

    puts("Test successful");
    return 0;

err:

    puts("test log failed");
    return -1;
}
