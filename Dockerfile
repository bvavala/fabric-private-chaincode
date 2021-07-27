FROM fpc/fpccc
ARG SGX_MODE
ENV SGX_MODE=${SGX_MODE}

RUN mkdir -p /project/_build
RUN mkdir -p /project/config
RUN mkdir -p /project/fabric/bin
RUN mkdir -p /project/common/crypto/attestation-api

ADD common/crypto/_build/ /project/_build/
ADD config/ /project/config/
ADD fabric/bin /project/fabric/bin
ADD common/crypto/attestation-api /project/common/crypto/attestation-api

WORKDIR /project/_build/tests

CMD ["bash", "./attested_evidence_test.sh"] 
