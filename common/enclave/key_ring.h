/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "sgx_tcrypto.h"

typedef enum
{
    KT_NO_KEY,
    KT_PUBLIC_ENCRYPTION_KEY,
    KT_PRIVATE_ENCRYPTION_KEY,
    KT_PUBLIC_SIGNING_KEY,
    KT_PRIVATE_SIGNING_KEY,
    KT_SGX_RSA_PUBLIC,
    KT_SGX_RSA_PRIVATE
} key_type_e;

class key_ring
{
    private:
        sgx_rsa3072_key_t decryption_key_;
        sgx_rsa3072_public_key_t encryption_key_;

    public:
        bool generate();
        const void* key_pointer(key_type_e key_type, size_t* p_size) const;
        bool to_b64(key_type_e kt, std::string& b64) const;
};

