# Copyright IBM Corp. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0

TOP = ..
include $(TOP)/build.mk

PROTO_BUILD_DIR := enclave/protos
BUILD_DIR := _build

nanopb:
ifndef NANOPB_PATH
	@echo "NANOPB_PATH undefined, check README"; exit 1
endif
	git clone https://github.com/nanopb/nanopb.git ${NANOPB_PATH}
	-cd ${NANOPB_PATH} && git checkout nanopb-0.3.9.2 && cd generator/proto && make

.PHONY: protos
protos: nanopb $(PROTO_BUILD_DIR)

$(PROTO_BUILD_DIR):
	./generate_protos.sh $@

.PHONY: protos-clean
protos-clean:
	-rm -rf $(PROTO_BUILD_DIR)

$(BUILD_DIR): protos
	mkdir -p $(BUILD_DIR) && \
		cd $(BUILD_DIR) && \
		cmake ./..

build: $(BUILD_DIR) protos
	$(MAKE) all --directory=$<

test:
	# make test verbose for now as tlcc doesn't abort on many failures just reports them in the logs ...
	$(MAKE) $@ --directory=$(BUILD_DIR) ARGS="-V"

gen_testcases:
	../integration/deployment_test.sh $(PWD)/test/getLedgerBlocks -o $(PWD)/test/test_blocks

.PHONY: enclave-clean
enclave-clean:
	-rm -rf $(BUILD_DIR)
	-rm -f enclave/*_t.*
	-rm -f trusted_ledger/*_u.*

clean: enclave-clean protos-clean
