/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "pdo/common/crypto/crypto.h"
#include "sgx_tcrypto.h"

typedef enum
{
    KT_NO_KEY,
    KT_ENCRYPTION,
    KT_DECRYPTION,
    KT_SIGNATURE,
    KT_VERIFICATION,
    KT_CC_ENCRYPTION,
    KT_CC_DECRYPTION
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
    bool serialize(key_type_e kt, std::string& serialized_key) const;
    bool to_public_proto(uint8_t* buf, size_t buf_size, size_t* out_size) const;
};
