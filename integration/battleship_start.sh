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

echo "Now you can play"
read -p "Press enter to shutdown the ledger and exit"

say "- shutdown ledger"
ledger_shutdown

exit 0

