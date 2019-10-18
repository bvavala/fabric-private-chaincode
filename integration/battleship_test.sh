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

# 1. prepare
para
say "Preparing Battleship ..."
# - clean up relevant docker images
docker_clean ${ERCC_ID}
docker_clean ${CC_ID}

trap ledger_shutdown EXIT

para

say "- setup ledger"
ledger_init

say "- battleship"

try ${PEER_CMD} chaincode install -l fpc-c -n ${CC_ID} -v ${CC_VERS} -p ${ENCLAVE_SO_PATH}
sleep 3

try ${PEER_CMD} chaincode instantiate -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -v ${CC_VERS} -c '{"Args":[""]}' -V ecc-vscc
sleep 3

try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"NewGame", "Args": [""]}' --waitForEvent
echo RESPONSE=$RESPONSE
check_result "OK"

#player one join
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"JoinGame", "Args": ["{\"ship_array\": [2, 0, 0, 0, 3, 1, 1, 0, 3, 2, 2, 0, 4, 3, 3, 0, 5, 4, 4, 0]} "]}' --waitForEvent
echo RESPONSE=$RESPONSE

#player two join
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"JoinGame", "Args": ["{\"ship_array\": [2, 0, 0, 0, 3, 1, 1, 0, 3, 2, 2, 0, 4, 3, 3, 0, 5, 4, 4, 0]} "]}' --waitForEvent
echo RESPONSE=$RESPONSE

try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"RefreshPlayer", "Args": ["{\"token_string\": \"AF\"}"]}' --waitForEvent
echo RESPONSE=$RESPONSE

try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"RefreshPlayer", "Args": ["{\"token_string\": \"CD\"}"]}' --waitForEvent
echo RESPONSE=$RESPONSE

#let's only sink a ship
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"Shoot", "Args": ["{\"x\": 0, \"y\": 0, \"token_string\": \"AF\"}"]}' --waitForEvent
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"Shoot", "Args": ["{\"x\": 0, \"y\": 0, \"token_string\": \"CD\"}"]}' --waitForEvent
try_r ${PEER_CMD} chaincode invoke -o ${ORDERER_ADDR} -C ${CHAN_ID} -n ${CC_ID} -c '{"Function":"Shoot", "Args": ["{\"x\": 0, \"y\": 1, \"token_string\": \"AF\"}"]}' --waitForEvent

say "- shutdown ledger"
ledger_shutdown

para
if [[ "$FAILURES" == 0 ]]; then
    yell "Battleship test PASSED"
else
    yell "Battleship test had ${FAILURES} failures"
    exit 1
fi
exit 0

