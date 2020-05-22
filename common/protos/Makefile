# Copyright 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

TOP = ../..
include $(TOP)/build.mk

CC_ENCLAVE_PROTO_PATH=${FPC_PATH}/common/protos

.PHONY: build
build:
	protoc "--plugin=protoc-gen-nanopb=${NANOPB_PATH}/generator/protoc-gen-nanopb" --proto_path=${CC_ENCLAVE_PROTO_PATH} "--nanopb_out=-f ${CC_ENCLAVE_PROTO_PATH}/credentials.options:${CC_ENCLAVE_PROTO_PATH}" ${CC_ENCLAVE_PROTO_PATH}/*.proto

	mkdir -p ${CC_ENCLAVE_PROTO_PATH}/go
	protoc -I=${CC_ENCLAVE_PROTO_PATH} --go_out=${CC_ENCLAVE_PROTO_PATH}/go ${CC_ENCLAVE_PROTO_PATH}/*.proto

clean:
	pushd ${CC_ENCLAVE_PROTO_PATH}; rm -rf *.c go *.h; popd

