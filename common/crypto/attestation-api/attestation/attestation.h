/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ATTESTATION_TYPE_TAG "attestation_type"
#define SIMULATED_TYPE_TAG "simulated"
#define EPID_LINKABLE_TYPE_TAG "epid-linkable"
#define EPID_UNLINKABLE_TYPE_TAG "epid-unlinkable"
#define ATTESTATION_TAG "attestation"
#define SPID_TAG "hex_spid"
#define SIG_RL_TAG "sig_rl"

bool init_attestation(uint8_t* params, uint32_t params_length);

bool get_attestation(uint8_t* statement,
    uint32_t statement_length,
    uint8_t* attestation,
    uint32_t attestation_max_length,
    uint32_t* attestation_length);
