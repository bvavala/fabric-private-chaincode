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
    // TODO create keys
    {
        //create the RSA keys
        unsigned char temp1[SGX_RSA3072_KEY_SIZE];
        unsigned char temp2[SGX_RSA3072_KEY_SIZE];
        unsigned char temp3[SGX_RSA3072_KEY_SIZE];
        unsigned char temp4[SGX_RSA3072_KEY_SIZE];
        unsigned char temp5[SGX_RSA3072_KEY_SIZE];
        //sgx_status_t ret = sgx_create_rsa_key_pair(
        //        SGX_RSA3072_KEY_SIZE,
        //        SGX_RSA3072_PUB_EXP_SIZE,
        //        decryption_key_.mod,
        //        decryption_key_.d,
        //        decryption_key_.e,
        //        (unsigned char*)&temp1,
        //        (unsigned char*)&temp2,
        //        (unsigned char*)&temp3,
        //        (unsigned char*)&temp4,
        //        (unsigned char*)&temp5);
        //LOG_DEBUG("sgx_create_rsa_key_pair %d", ret);
        //COND2ERR(SGX_SUCCESS != ret);
        for(unsigned int i=0; i<sizeof(decryption_key_); i++)
        {
            char* p = (char*)&decryption_key_;
            *p = (char)i;
        }

        memcpy_s(encryption_key_.mod, sizeof(encryption_key_.mod),
                decryption_key_.mod, sizeof(decryption_key_.mod));
        memcpy_s(encryption_key_.exp, sizeof(encryption_key_.exp),
                decryption_key_.e, sizeof(decryption_key_.e));
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
