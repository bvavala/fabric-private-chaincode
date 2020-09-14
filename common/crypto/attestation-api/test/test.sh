# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# *** README ***
# This script is meant to run as part of the build.
# The script is transfered to the folder where other test binaries will be located,
# and it will orchestrate the test.
# Orchestration involves: preparing input file for init_attestation,
# calling get_attestation, calling verify_evidence.

set -e

. ../scripts/attestation_to_evidence.sh
. ../scripts/enclave_to_mrenclave.sh

function orchestrate()
{
    echo "orches.."
    #get attestation
    ./get_attestation_app
    OUTPUT_FILE="get_attestation_output.txt"
    [ -f ${OUTPUT_FILE} ] || die "no output from get_attestation"

    #translate attestation
    ATTESTATION=$(cat ${OUTPUT_FILE})
    attestation_to_evidence "${ATTESTATION}"
    INPUT_FILE="verify_evidence_input.txt"
    echo $EVIDENCE > $INPUT_FILE

    echo "checkin ..."
    #verify evidence
    ./verify_evidence_app

    echo "att complete"

    #clean artifacts
    rm -rf *.txt
    echo "orche done"
}

#######################################
# sim mode test
#######################################

echo "Testing simulated attestation"

#prepare input
echo "this is ignored" > code_id.txt
echo "1234567890" > statement.txt
echo "{\"attestation_type\": \"simulated\"}" > init_attestation_input.txt

orchestrate

echo "Test simulated attestation success"

#######################################
# hw mode test
#######################################
if [[ ${SGX_MODE} == "HW" ]]; then
    echo "Testing actual attestation"

    enclave_to_mrenclave libtest_enclave.so test_enclave.config.xml
    echo -n "$MRENCLAVE" > code_id.txt
    echo -n "1234567890" > statement.txt

    #get api key
    SPID_FILEPATH="${FPC_PATH}/config/ias/spid.txt"
    SPID_TYPE_FILEPATH="${FPC_PATH}/config/ias/spid_type.txt"

    test -f ${SPID_FILEPATH} || die "no spid file ${SPID_FILEPATH}"
    test -f ${SPID_TYPE_FILEPATH} || die "no spid type file ${SPID_TYPE_FILEPATH}"
    SPID=$(cat $SPID_FILEPATH)
    SPID_TYPE=$(cat $SPID_TYPE_FILEPATH)

    #prepare input
    echo "{\"attestation_type\": \"$SPID_TYPE\", \"hex_spid\": \"$SPID\", \"sig_rl\":\"\"}" > init_attestation_input.txt

    orchestrate

else
    echo "Skipping actual attestation test"
fi

echo "Test successful."
exit 0
