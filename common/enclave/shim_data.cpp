/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "shim_data.h"
#include <string>
#include "logging.h"
/******************************************************************************
 * The channel id is set once, and then maintained fixed for consistency.
 * The value is meant to be either provided by the Fabric shim on enclave
 * creation, or set after unsealing the chaincode parameters.
 * In both cases, the value is not verified. The verification is performed
 * by the Enclave Registry when the chaincode/enclave parameters are registered.
 *****************************************************************************/

static char g_channel_id[MAX_CHANNEL_ID_LENGTH];
static uint32_t g_channel_id_length;
static bool g_channel_id_set = false;

bool shim_data_set_channel_id(const char* channel_id, uint32_t channel_id_length)
{
    if (g_channel_id_set)
    {
        LOG_ERROR("channel id already set");
        return false;
    }

    if (channel_id_length + 1 > MAX_CHANNEL_ID_LENGTH)
    {
        LOG_ERROR("channel id %s (length %u) too long", channel_id_length);
        return false;
    }

    strncpy(g_channel_id, channel_id, channel_id_length);
    g_channel_id_length = channel_id_length;
    g_channel_id[g_channel_id_length + 1] = '\0';
    g_channel_id_set = true;
    return true;
}

bool shim_data_get_channel_id(char* channel_id, uint32_t max_channel_id_length)
{
    if (!g_channel_id_set)
    {
        LOG_ERROR("channel id not set");
        return false;
    }

    if (max_channel_id_length < g_channel_id_length + 1)
    {
        LOG_ERROR("input channel id buffer length is insufficient");
        return false;
    }

    strncpy(channel_id, g_channel_id, g_channel_id_length);
    channel_id[g_channel_id_length + 1] = '\0';
    return true;
}

/******************************************************************************
 * The msp id is set once, and then maintained fixed for consistency.
 * The value is meant to be either provided by the Fabric shim on enclave
 * creation, or set after unsealing the chaincode parameters.
 * In both cases, the value is not verified. The verification is performed
 * by the Enclave Registry when the chaincode/enclave parameters are registered.
 *****************************************************************************/

static char g_msp_id[MAX_MSP_ID_LENGTH];
static uint32_t g_msp_id_length;
static bool g_msp_id_set = false;

bool shim_data_set_msp_id(const char* msp_id, uint32_t msp_id_length)
{
    if (g_msp_id_set)
    {
        LOG_ERROR("msp id already set");
        return false;
    }

    if (msp_id_length + 1 > MAX_CHANNEL_ID_LENGTH)
    {
        LOG_ERROR("msp id %s (length %u) too long", msp_id_length);
        return false;
    }

    strncpy(g_msp_id, msp_id, msp_id_length);
    g_msp_id_length = msp_id_length;
    g_msp_id[g_msp_id_length + 1] = '\0';
    g_msp_id_set = true;
    return true;
}

bool shim_data_get_msp_id(char* msp_id, uint32_t max_msp_id_length)
{
    if (!g_msp_id_set)
    {
        LOG_ERROR("channel id not set");
        return false;
    }

    if (max_msp_id_length < g_msp_id_length + 1)
    {
        LOG_ERROR("input msp id buffer length is insufficient");
        return false;
    }

    strncpy(msp_id, g_msp_id, g_msp_id_length);
    msp_id[g_msp_id_length + 1] = '\0';
    return true;
}
