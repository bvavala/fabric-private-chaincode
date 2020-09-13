# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

set -e

#######################################
# sim mode test
#######################################

echo "Testing simulated attestation"
#prepare input
echo "{\"attestation_type\": \"simulated\"}" > init_attestation_input.txt
#get attestation
./get_attestation
#translate attestation
#verify evidence


#######################################
# hw mode test
#######################################
if [[ ${SGX_MODE} == "HW" ]]; then
    echo "Testing actual attestation"
else
    echo "Skipping actual attestation test"
fi
