/* Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "key_ring.h"
#include <pb_encode.h>
#include "base64.h"
#include "error.h"
#include "logging.h"
#include "protos/credentials.pb.h"

bool key_ring::generate()
{
    // create keys
    {
        try
        {
            signature_key_.Generate();  // private key
            verification_key_ = signature_key_.GetPublicKey();
            decryption_key_.Generate();  // private key
            encryption_key_ = decryption_key_.GetPublicKey();
            cc_decryption_key_.Generate();  // private key
            cc_encryption_key_ = cc_decryption_key_.GetPublicKey();

            // debug
            std::string s = verification_key_.Serialize();
            LOG_DEBUG("enclave verification key: %s", s.c_str());
        }
        catch (...)
        {
            LOG_ERROR("error creating cryptographic keys");
            return false;
        }
    }
    LOG_DEBUG("key-ring generate success");
    return true;

err:
    return false;
}

bool key_ring::serialize(key_type_e kt, std::string& serialized_key) const
{
    try
    {
        switch (kt)
        {
            case KT_ENCRYPTION:
            {
                serialized_key = encryption_key_.Serialize();
                break;
            }
            case KT_DECRYPTION:
            {
                serialized_key = decryption_key_.Serialize();
                break;
            }
            case KT_SIGNATURE:
            {
                serialized_key = signature_key_.Serialize();
                break;
            }
            case KT_VERIFICATION:
            {
                serialized_key = verification_key_.Serialize();
                break;
            }
            case KT_CC_ENCRYPTION:
            {
                serialized_key = cc_encryption_key_.Serialize();
                break;
            }
            case KT_CC_DECRYPTION:
            {
                serialized_key = cc_decryption_key_.Serialize();
                break;
            }
            default:
            {
                LOG_ERROR("bad key type %d", kt);
                return false;
            }
        }
    }
    catch (...)
    {
        LOG_ERROR("error serializing key type %d", kt);
        return false;
    }

    return true;
}

bool key_ring::to_public_proto(uint8_t* buf, size_t buf_size, size_t* out_size) const
{
    pb_ostream_t ostream = pb_ostream_from_buffer(buf, buf_size);
    {
        // verification key
        std::string s;
        COND2ERR(!serialize(KT_VERIFICATION, s));
        COND2ERR(!pb_encode_tag(&ostream, PB_WT_STRING, enclavePublicKeys_enclaveVerifyingKey_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)s.c_str(), s.length() + 1));
    }
    {
        // encryption key
        std::string s;
        COND2ERR(!serialize(KT_ENCRYPTION, s));
        COND2ERR(
            !pb_encode_tag(&ostream, PB_WT_STRING, enclavePublicKeys_enclaveEncryptionKey_tag));
        COND2ERR(!pb_encode_string(&ostream, (const unsigned char*)s.c_str(), s.length() + 1));
    }

    *out_size = ostream.bytes_written;
    LOG_DEBUG("key ring public proto size %d", *out_size);
    return true;

err:
    return false;
}
