#!/bin/bash

# Copyright Intel Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
set -x
SCRIPTDIR="$(dirname $(readlink --canonicalize ${BASH_SOURCE}))"
FPC_TOP_DIR="${SCRIPTDIR}/.."
FABRIC_SCRIPTDIR="${FPC_TOP_DIR}/fabric/bin/"

: ${FABRIC_CFG_PATH:="${SCRIPTDIR}/config"}

. ${FABRIC_SCRIPTDIR}/lib/common_utils.sh
. ${FABRIC_SCRIPTDIR}/lib/common_ledger.sh

CC_ID=echo_test

#this is the path that will be used for the docker build of the chaincode enclave
ENCLAVE_SO_PATH=examples/echo/_build/lib/

CC_VERS=0
num_rounds=1
FAILURES=0

IAS_CREDENTIAL_PATH=${FPC_PATH}/integration/config/ias
SPID="`cat $IAS_CREDENTIAL_PATH/spid.txt`"
echo SPID=$SPID
sleep 3

echo_test() {
    # install, init, and register (auction) chaincode
    try ${PEER_CMD} chaincode install -l fpc-c -n ${CC_ID} -v ${CC_VERS} -p ${ENCLAVE_SO_PATH}
    sleep 3

    try ${PEER_CMD} chaincode instantiate -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -v ${CC_VERS} -c '{"Args":[]}' -V ecc-vscc
    sleep 3

    say "do echos"
    for (( i=1; i<=$num_rounds; i++ ))
    do
        # echos
        #try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Args": ["echo-'$i'"]}' --waitForEvent
        #check_result "echo-$i"
        try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Args": ["ctl_create", "'$SPID'"]}'
        echo "ctl_create: $RESPONSE"
        CI_RESPONSE=${RESPONSE}
        CI_RESPONSE=${CI_RESPONSE##*ResponseData\\\":\\\"}
        CI_RESPONSE=${CI_RESPONSE%%\\*}
        # Convert and de-encrypt it
        CREDENTIALS=$(${ENCLAVE_MANAGER_CMD} "`echo ${CI_RESPONSE} | base64 -d | base64 --wrap=0`")
        echo creds=$CREDENTIALS
        echo $CREDENTIALS | base64 -d --wrap=0

        try ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${ERCC_ID} -c '{"args":["newEnclave", "'$CREDENTIALS'"]}'

     done
}

# 1. prepare
para
say "Preparing Echo Test ..."
# - clean up relevant docker images
docker_clean ${ERCC_ID}
docker_clean ${CC_ID}

trap ledger_shutdown EXIT


para
say "Run echo test"

say "- setup ledger"
ledger_init

say "- echo test"
echo_test

say "- shutdown ledger"
ledger_shutdown

para
if [[ "$FAILURES" == 0 ]]; then
    yell "Echo test PASSED"
else
    yell "Echo test had ${FAILURES} failures"
    exit 1
fi
exit 0

