#!/bin/bash

# Copyright Intel Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

SCRIPTDIR="$(dirname $(readlink --canonicalize ${BASH_SOURCE}))"
FPC_TOP_DIR="${SCRIPTDIR}/.."
FABRIC_SCRIPTDIR="${FPC_TOP_DIR}/fabric/bin/"

: ${FABRIC_CFG_PATH:="${SCRIPTDIR}/config"}

. ${FABRIC_SCRIPTDIR}/lib/common_utils.sh
. ${FABRIC_SCRIPTDIR}/lib/common_ledger.sh

CC_ID=battleship_cc

#this is the path that will be used for the docker build of the chaincode enclave
ENCLAVE_SO_PATH=examples/battleship/_build/lib/

CC_VERS=0
num_rounds=3
num_clients=10
FAILURES=0

#the first parameter is the function, the second one is the json blob with the battleship data, third is file to write output in

set -x
echo "parameters $1 $2 $3"
CC_CMD_STRING="{\"Function\":\"$1\",\"Args\":[\"$2\"]}"
#try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"$1", "Args": ["$2"]}' --waitForEvent
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c $CC_CMD_STRING --waitForEvent

#check commit status, if invalid, put error message in file and exit
CI_RESPONSE=${RESPONSE}
CI_RESPONSE=${CI_RESPONSE##*committed with status (}
CI_RESPONSE=${CI_RESPONSE%%)*}
if [ $CI_RESPONSE != "VALID" ]; then
    echo "commit error: $CI_RESPONSE" > $3
    exit 0
fi

#if commit is valid, put response in file
CI_RESPONSE=${RESPONSE}
CI_RESPONSE=${CI_RESPONSE##*ResponseData\\\":\\\"}
CI_RESPONSE=${CI_RESPONSE%%\\*}
# Convert and de-encrypt it
CI_RESPONSE=$(echo ${CI_RESPONSE} | base64 -d)
echo $CI_RESPONSE > $3
echo $3

exit 0

