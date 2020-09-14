#!/bin/bash

# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

set -e

if [[ -z "${FPC_PATH}" ]]; then
    echo "FPC_PATH not set"
    exit -1
fi

. ${FPC_PATH}/fabric/bin/lib/common_utils.sh

function b64quote_to_iasresponse() {
    #get api key
    API_KEY_FILEPATH="${FPC_PATH}/config/ias/api_key.txt"
    test -f ${API_KEY_FILEPATH} || die "no api key file ${API_KEY_FILEPATH}"
    API_KEY=$(cat $API_KEY_FILEPATH)

    #get verification report
    QUOTE=$1
    # contact IAS to get the verification report
    IAS_RESPONSE=$(curl -s -H "Content-Type: application/json" -H "Ocp-Apim-Subscription-Key:$API_KEY" -X POST -d '{"isvEnclaveQuote":"'$QUOTE'"}' https://api.trustedservices.intel.com/sgx/dev/attestation/v4/report -i)
    # check status (as there may be multiple header, we rather check presence of relevant fields)
    echo "$IAS_RESPONSE" | grep "X-IASReport-Signature"           >/dev/null || die "IAS Response error"
    echo "$IAS_RESPONSE" | grep "X-IASReport-Signing-Certificate" >/dev/null || die "IAS Response error"
}

function iasresponse_to_evidence() {
    IAS_RESPONSE="$1"
    #encode relevant info in json format
    IAS_SIGNATURE=$(echo "$IAS_RESPONSE" | grep -Po 'X-IASReport-Signature: \K[^ ]+' | tr -d '\r')
    IAS_CERTIFICATES=$(echo "$IAS_RESPONSE" | grep -Po 'X-IASReport-Signing-Certificate: \K[^ ]+')
    IAS_REPORT=$(echo "$IAS_RESPONSE" | grep -Po '{"id":[^ ]+')
    JSON_IAS_RESPONSE=$(jq -c -n --arg sig "$IAS_SIGNATURE" --arg cer "$IAS_CERTIFICATES" --arg rep "$IAS_REPORT" '{iasSignature: $sig, iasCertificates: $cer, iasReport: $rep}')
    #IAS_EVIDENCE=$(echo "$JSON_IAS_RESPONSE" | base64 --wrap=0)
    IAS_EVIDENCE=$JSON_IAS_RESPONSE
}


function simulated_to_evidence() {
    SIMULATED_EVIDENCE=$1
}

ATTESTATION_TYPE_TAG="attestation_type"
ATTESTATION_TAG="attestation"
EVIDENCE_TAG="evidence"

function attestation_to_evidence() {
    if [[ -z "$1" ]]; then
        die "no argument provided"
    fi
    echo "attestation_to_evidence of $1"

    ATTESTATION_TYPE=$(echo $1 | jq ".$ATTESTATION_TYPE_TAG" -r)
    ATTESTATION=$(echo $1 | jq ".$ATTESTATION_TAG" -r)

    case "$ATTESTATION_TYPE" in
        simulated)
            echo "simulated"
            simulated_to_evidence "$ATTESTATION"
            EVIDENCE=$SIMULATED_EVIDENCE
            ;;

        epid-linkable)
            ;&
        epid-unlinkable)
            echo "epid"
            b64quote_to_iasresponse "$ATTESTATION"
            echo "iasresponse: $IAS_RESPONSE"
            iasresponse_to_evidence "$IAS_RESPONSE"
            EVIDENCE=$IAS_EVIDENCE
            ;;
        *)
            die "error attestation type $ATTESTATION_TYPE"
            ;;
    esac

    #package evidence
    EVIDENCE=$(jq -c -n --arg attestation_type "$ATTESTATION_TYPE" --arg evidence "$EVIDENCE" '{'$ATTESTATION_TYPE_TAG': $attestation_type, '$EVIDENCE_TAG': $evidence}')

    echo "Evidence: $EVIDENCE"
}

#B64QUOTE="AgABAFQLAAALAAoAAAAAAPTVKSgcztOrYJKkNQirk3YAAAAAAAAAAAAAAAAAAAAABg7/BQGAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABwAAAAAAAAAHAAAAAAAAAD4PP/tEC54xfE9zEqDWRP/AUvdkU1BWTUNYbYECZjNYAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMQiQgL2eL4ktG4oR/x5zgl6LZ7MgrUAsRqf89lu6SVAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHdee3V+3mMM0KoRE70QJmGrOIKcpSpkIqt4KGLyaGRgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAqAIAALcbaKDJmokAzEp3xSx6bxnaNQWlyZcg1wzj5+bqwjivFa/TaEVFnhnNR60kibeQydjFRpYXpoXOeITS98w32UUAJUejac240vxzEENXKdGrCb2oZd3NUL5EoTWjoV5Dgy//scP+WqygBgQMLfYSRru3ROx68Fr1RQb1IT2nmKSEzUFKRVNXxWQu3ACUwFs+3mq5AgkKd8n3easrXvOMLmbb7MS7cElsyGY6d9cjJxX6+q+pTsUJgMZ4ipvsf2Dkh+DebS+/W2Fb1QwU68nvRB68PzP5S4TL3JWRGvKe6rxGnSU2uir1lJgkGFelEWw+6RLC596Boh4NEdZ9xgbW+Uoi1BwXamRDKMPrlLs2F6bwY+wYkejngth8t23bGoEP0s2aVksbl8LaTP3VM2gBAAA4NfAlCm1YPDpkFFHKgwlFp7hShlmqsvsXMGRfMgIxI/eAO+kyRI4HXadJhiB3KOJaaZXSZ0U24ViHfwvPAJ+pF4UmlG8gmx8wDfX0ThKhtyaMlJnSUvkwXGNMSKcCsSibmtndXQFGpmWTbNk1Y4GqGXy4pTDz7/6qDI9rQuN4gtmQo8KunlSrMgwM9vqBNWprNDrqoJEmGUyRBF3qqB4D78wi+zsAFRZ926Pk7wHBpIkx8zIBAyIOHyLm6H47zC+mggFUmhozcy6hYQetucKCdC0ZBV6/Av3APKiiSl2kBXtPHLZeVelSxIhB8ZOr0kvr3qr+ilM1ZG3DdwAe5GZlbCPZaCeZgGORiPEQth7xCqK4V7FSdyT+HlRvgDoJC3kirlM+ji9wHyQue94wybtrZg4OToAGCxHMNFC+pF/nrMC7tfo+m5KZpPyZ35N7tdgPIZyuFoRo7ikZYIsnA54av4jv7BkOUHQNFgAZ3c8GPzCYgw2UUXxG"

#b64quote_to_iasresponse "$B64QUOTE"
#echo "iasresponse: $IAS_RESPONSE"
#iasresponse_to_evidence "$IAS_RESPONSE"
#echo "evidence: $IAS_EVIDENCE"
