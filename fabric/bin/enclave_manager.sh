#!/bin/bash
# Copyright 2020 Intel Corp.
#
# SPDX-License-Identifier: Apache-2.0

set -e

if [[ -z "$FPC_PATH" ]]; then
    echo "FPC_PATH not set"
fi

if [[ -z "$1" ]]; then
    echo "no argument provided"
fi

IAS_CREDENTIAL_PATH=${FPC_PATH}/integration/config/ias
API_KEY="`cat $IAS_CREDENTIAL_PATH/api_key.txt`"

# first argument is the base64-encoded protobuf received by the ecc enclave
B64_CREDENTIAL_PB=$1

#create temporary file
TMP_MESSAGE_FILE=$(mktemp)
trap "{ rm -f $TMP_MESSAGE_FILE; }" EXIT

#decode the message
echo "$B64_CREDENTIAL_PB" | base64 -d | protoc --decode credentials --proto_path=$FPC_PATH/common/protos/chaincode_enclave $FPC_PATH/common/protos/chaincode_enclave/credentials.proto > $TMP_MESSAGE_FILE

#grab the quote field
ENCLAVE_QUOTE=$(cat $TMP_MESSAGE_FILE | grep enclaveAttestation | sed 's/enclaveAttestation: //' | sed 's/"//g')

#contact IAS to get the verification report
IAS_RESPONSE=$(curl -s -H "Content-Type: application/json" -H "Ocp-Apim-Subscription-Key:$API_KEY" -X POST -d '{"isvEnclaveQuote":"'$ENCLAVE_QUOTE'"}'  https://api.trustedservices.intel.com/sgx/dev/attestation/v3/report -i)

#encode relevant info in json format
IAS_SIGNATURE=$(echo $IAS_RESPONSE | grep -Po 'X-IASReport-Signature: \K[^ ]+')
IAS_CERTIFICATES=$(echo $IAS_RESPONSE | grep -Po 'X-IASReport-Signing-Certificate: \K[^ ]+')
IAS_REPORT=$(echo $IAS_RESPONSE | grep -Po '{"id":[^ ]+')
JSON_IAS_RESPONSE=$(jq -n --arg sig "$IAS_SIGNATURE" --arg cer "$IAS_CERTIFICATES" --arg rep "$IAS_REPORT" '{iasSignature: $sig, iasCertificates: $cer, iasReport: $rep}' | base64 --wrap=0)
#JSON_IAS_RESPONSE=$(jq -n --arg sig "$IAS_SIGNATURE" --arg rep "$IAS_REPORT" '{iasSignature: $sig, iasReport: $rep}' | base64 --wrap=0)

#append verification report in message file
#NOTICE: the "verificationReport" string is defined in the proto file
#echo "reportDataFields: \"\"" >> $TMP_MESSAGE_FILE
echo "verificationReport: \"$JSON_IAS_RESPONSE\"" >> $TMP_MESSAGE_FILE

#encode the message file as protobuf, and output its base64-encoding
protoc --encode credentials --proto_path=$FPC_PATH/common/protos/chaincode_enclave $FPC_PATH/common/protos/chaincode_enclave/credentials.proto < $TMP_MESSAGE_FILE | base64 --wrap=0

