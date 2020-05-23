/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "sgx_tcrypto.h"
#include "pdo/common/crypto/crypto.h"

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
        pdo::crypto::sig::PublicKey verification_key_;
        pdo::crypto::sig::PrivateKey signature_key_;
        pdo::crypto::pkenc::PublicKey encryption_key_;
        pdo::crypto::pkenc::PrivateKey decryption_key_;
        pdo::crypto::pkenc::PublicKey cc_encryption_key_;
        pdo::crypto::pkenc::PrivateKey cc_decryption_key_;

    public:
        bool generate();
        const void* key_pointer(key_type_e key_type, size_t* p_size) const;
        bool to_b64(key_type_e kt, std::string& b64) const;
};

