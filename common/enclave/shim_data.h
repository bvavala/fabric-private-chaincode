/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fpc-types.h"

#define MAX_CHANNEL_ID_LENGTH 1024
#define MAX_MSP_ID_LENGTH 1024

bool shim_data_set_channel_id(const char* channel_id, uint32_t channel_id_length);
bool shim_data_get_channel_id(char* channel_id, uint32_t max_channel_id_length);

bool shim_data_set_msp_id(const char* msp_id, uint32_t msp_id_length);
bool shim_data_get_msp_id(char* msp_id, uint32_t max_msp_id_length);
