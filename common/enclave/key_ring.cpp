/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "key_ring.h"
#include "error.h"
#include "logging.h"
#include "base64.h"
#include "c11_support.h"

bool key_ring::generate()
{
    // create keys
    {
        try
        {
            signature_key_.Generate(); //private key
            verification_key_ = signature_key_.GetPublicKey();
            decryption_key_.Generate(); //private key
            encryption_key_ = decryption_key_.GetPublicKey();
            cc_decryption_key_.Generate(); //private key
            cc_encryption_key_ = cc_decryption_key_.GetPublicKey();

            //debug
            std::string s = verification_key_.Serialize();
            LOG_DEBUG("enclave verification key: %s", s.c_str());
        }
        catch(...)
        {
            LOG_ERROR("error creating cryptographic keys");
            return false;
        }
    }
    LOG_DEBUG("keyring generate success");
    return true;

err:
    return false;
}

const void* key_ring::key_pointer(key_type_e kt, size_t* size) const
{
    switch(kt)
    {
        case KT_SGX_RSA_PUBLIC:
            {
                *size = sizeof(encryption_key_);
                return (const void*)&encryption_key_;
            }
        case KT_SGX_RSA_PRIVATE:
            {
                *size = sizeof(decryption_key_);
                return (const void*)&decryption_key_;
            }
        default:
            break;
    }
    return NULL;
}

bool key_ring::to_b64(key_type_e kt, std::string& b64) const
{
    size_t size;
    const void *p = key_pointer(kt, &size);
    COND2ERR(p == NULL);
    b64 = base64_encode((const unsigned char*)p, size);
    return true;

err:
    return false;
}

