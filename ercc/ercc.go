/*
Copyright IBM Corp. All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package ercc

import (
	"strings"

	"crypto/sha256"
	"encoding/base64"
	"encoding/json"

	"github.com/hyperledger-labs/fabric-private-chaincode/ercc/attestation"
	"github.com/hyperledger-labs/fabric-private-chaincode/ercc/attestation/mock"
	"github.com/hyperledger/fabric/core/chaincode/shim"
	pb "github.com/hyperledger/fabric/protos/peer"

    "github.com/golang/protobuf/proto"
    cred "github.com/hyperledger-labs/fabric-private-chaincode/common/protos/chaincode_enclave/go"
)

// #cgo CFLAGS: -I/opt/intel/sgxsdk/include -I${SRCDIR}/attestation/verify_ias_report
// #cgo LDFLAGS: -L${SRCDIR}/attestation/_build -lverifyiasreport -lcrypto
// #include "verify-report.h"
import "C"

var logger = shim.NewLogger("ercc")

// EnclaveRegistryCC ...
type EnclaveRegistryCC struct {
	ra  attestation.Verifier
	ias attestation.IntelAttestationService
}

// NewErcc is a helpful factory method for creating this beauty
func NewErcc() *EnclaveRegistryCC {
	logger.Debug("NewErcc called")
	return &EnclaveRegistryCC{
		ra:  GetVerifier(),
		ias: GetIAS(),
	}
}

func NewTestErcc() *EnclaveRegistryCC {
	return &EnclaveRegistryCC{
		ra:  &mock.MockVerifier{},
		ias: &mock.MockIAS{},
	}
}

// Init setups the EnclaveRegistry by initializing intel verification key. Currently this is hardcoded!
func (ercc *EnclaveRegistryCC) Init(stub shim.ChaincodeStubInterface) pb.Response {
	logger.Debug("Init called")
	return shim.Success(nil)
}

// Invoke receives transactions and forwards to op handlers
func (ercc *EnclaveRegistryCC) Invoke(stub shim.ChaincodeStubInterface) pb.Response {
	function, args := stub.GetFunctionAndParameters()
	logger.Debugf("Invoke(function=%s, %s) called", function, args)

	if function == "registerEnclave" {
		return ercc.registerEnclave(stub, args)
	} else if function == "getAttestationReport" { //get enclave attestation report
		return ercc.getAttestationReport(stub, args)
	} else if function == "getSPID" { //get SPID
		return ercc.getSPID(stub, args)
	} else if function == "test_ias" {
        ret := C.test_ias()
        logger.Debugf("test ias %d", int(ret))
        a := []byte{'c', 'i', 'a', 'o'}
        return shim.Success(a)
    } else if function == "newEnclave" {
        return ercc.newEnclave(stub, args)
    }

	return shim.Error("Received unknown function invocation: " + function)
}

// ============================================================
// newEnclave
// ============================================================
func (ercc *EnclaveRegistryCC) newEnclave(stub shim.ChaincodeStubInterface, args []string) pb.Response {
    logger.Debugf("ERCC: new enclave");
    logger.Debugf("ercc arg %s", args[0])
    pbBytes, err := base64.StdEncoding.DecodeString(args[0])
    if err != nil {
        return shim.Error("Cannot retrieve protobuf: " + err.Error())
    }

    logger.Debugf("ercc empty cred")

    credentials := &cred.Credentials{}
    logger.Debugf("ercc parse protobuf")
    if err := proto.Unmarshal(pbBytes, credentials); err != nil {
        logger.Debugf("ercc parse protobuf error")
        return shim.Error("Cannot parse protobuf: " + err.Error())
    }

    logger.Debugf("ercc get verb")
    verb := string(credentials.GetVerb())
    //if verb == nil {
    //    return shim.Error("verb missing");
    //}
    logger.Debugf("verb: %s", verb)

    return shim.Success(nil)
}

// ============================================================
// registerEnclave -
// ============================================================
func (ercc *EnclaveRegistryCC) registerEnclave(stub shim.ChaincodeStubInterface, args []string) pb.Response {
	// args:
	// 0: enclavePkBase64
	// 1: quoteBase64
	// 2: apiKey
	// if apiKey not available as argument we try to read them from decorator

	if len(args) < 2 {
		return shim.Error("Incorrect number of arguments. Expecting enclave pk and quote to register")
	}

	enclavePkAsBytes, err := base64.StdEncoding.DecodeString(args[0])
	if err != nil {
		return shim.Error("Can not parse enclavePkHash: " + err.Error())
	}

	quoteBase64 := args[1]
	quoteAsBytes, err := base64.StdEncoding.DecodeString(quoteBase64)
	if err != nil {
		return shim.Error("Can not parse quoteBase64 string: " + err.Error())
	}

	// get ercc api key for IAS
	var apiKey string
	if len(args) >= 3 {
		apiKey = args[2]
	} else {
		apiKey = string(stub.GetDecorations()["apiKey"])
	}
	apiKey = strings.TrimSpace(apiKey) // make sure there are no trailing newlines and alike ..
	logger.Debugf("registerEnclave: api-key: %s / len(args)=%d", apiKey, len(args))

	// send quote to intel for verification
	attestationReport, err := ercc.ias.RequestAttestationReport(apiKey, quoteAsBytes)
	if err != nil {
		return shim.Error("Error while retrieving attestation report: " + err.Error())
	}

	// TODO get verification public key from ledger
	verificationPK, err := ercc.ias.GetIntelVerificationKey()
	if err != nil {
		return shim.Error("Can not parse verifiaction key: " + err.Error())
	}

	// verify attestation report
	isValid, err := ercc.ra.VerifyAttestationReport(verificationPK, attestationReport)
	if err != nil {
		return shim.Error("Error while attestation report verification: " + err.Error())
	}
	if !isValid {
		return shim.Error("Attestation report is not valid")
	}

	// first verify that enclavePkHash matches the one in the attestation report
	isValid, err = ercc.ra.CheckEnclavePkHash(enclavePkAsBytes, attestationReport)
	if err != nil {
		return shim.Error("Error while checking enclave PK: " + err.Error())
	}
	if !isValid {
		return shim.Error("Enclave PK does not match attestation report!")
	}
	// set enclave public key in attestation report
	attestationReport.EnclavePk = enclavePkAsBytes

	// store attestation report under enclavePk hash in state
	attestationReportAsBytes, err := json.Marshal(attestationReport)
	if err != nil {
		return shim.Error(err.Error())
	}

	// create hash of enclave pk
	enclavePkHash := sha256.Sum256(enclavePkAsBytes)
	enclavePkHashBase64 := base64.StdEncoding.EncodeToString(enclavePkHash[:])
	err = stub.PutState(enclavePkHashBase64, attestationReportAsBytes)

	return shim.Success(nil)
}

// ============================================================
// getAttestationReport -
// ============================================================
func (ercc *EnclaveRegistryCC) getAttestationReport(stub shim.ChaincodeStubInterface, args []string) pb.Response {
	var err error

	//   0
	// "enclavePkHashBase64"
	if len(args) != 1 {
		return shim.Error("Incorrect number of arguments. Expecting pk of the enclave to query")
	}

	enclavePkHashBase64 := args[0]
	attestationReport, err := stub.GetState(enclavePkHashBase64) //get attestationREPORT for enclavePK from chaincode state
	if err != nil {
		return shim.Error("Failed to get state for " + enclavePkHashBase64)
	} else if attestationReport == nil {
		return shim.Error("EnclavePK does not exist: " + enclavePkHashBase64)
	}

	return shim.Success(attestationReport)
}

// ============================================================
// getSPID -
// ============================================================
func (ercc *EnclaveRegistryCC) getSPID(stub shim.ChaincodeStubInterface, args []string) pb.Response {
	// return spid from IASCredentialProvider
	return shim.Success(stub.GetDecorations()["SPID"])
	// return shim.Success(ercc.iascp.GetSPID())
}
