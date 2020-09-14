/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test.h"
#include <string>
#include <string.h>
#include "base64.h"
#include "error.h"
#include "logging.h"

#include "verify-evidence.h"
#include "attestation_tags.h"

#define STATEMENT_FILE "statement.txt"
#define CODE_ID_FILE "code_id.txt"
#define EVIDENCE_FILE "verify_evidence_input.txt"

bool load_file(const char *filename, char* buffer, uint32_t buffer_length)
{
    char* p;
    buffer[0] = '\0';
    FILE* fp = fopen(filename, "r");
    COND2LOGERR(fp == NULL, "can't open file");
    p = fgets(buffer, buffer_length, fp);
    LOG_DEBUG("read (len=%u, max=%u): %s\n", strlen(buffer), buffer_length, buffer);
    COND2LOGERR(p == NULL, "error fgets");
    //COND2LOGERR(!feof(fp), "EOF not reached");
    COND2LOGERR(strlen(buffer) >= buffer_length -1, "buffer too small");
    fclose(fp);
    return true;
err:
    return false;
}

bool test()
{
    uint32_t buffer_length = 1 << 20;
    char buffer[buffer_length];
    std::string jsonevidence;
    std::string expected_statement;
    std::string expected_code_id;
    std::string wrong_expected_statement;
    std::string wrong_expected_code_id;

    COND2LOGERR(!load_file(EVIDENCE_FILE, buffer, buffer_length), "can't read input evidence " EVIDENCE_FILE); 
    jsonevidence = std::string(buffer);

    COND2LOGERR(!load_file(STATEMENT_FILE, buffer, buffer_length), "can't read input statement " STATEMENT_FILE);
    expected_statement = std::string(buffer);

    COND2LOGERR(!load_file(CODE_ID_FILE, buffer, buffer_length), "can't read input code id " CODE_ID_FILE);
    expected_code_id = std::string(buffer);

    wrong_expected_statement = std::string("wrong statement");
    wrong_expected_code_id = std::string(
        "BADBADBADBAD9E317C4F7312A0D644FFC052F7645350564D43586D8102663358");

    bool b, expected_b;
    // test normal situation
    b = verify_evidence((uint8_t*)jsonevidence.c_str(), jsonevidence.length(),
        (uint8_t*)expected_statement.c_str(), expected_statement.length(),
        (uint8_t*)expected_code_id.c_str(), expected_code_id.length());
    COND2LOGERR(!b, "correct evidence failed");

    // this test succeeds for simulated attestations, and fails for real ones
    // test with wrong statement
    b = verify_evidence((uint8_t*)jsonevidence.c_str(), jsonevidence.length(),
        (uint8_t*)wrong_expected_statement.c_str(), wrong_expected_statement.length(),
        (uint8_t*)expected_code_id.c_str(), expected_code_id.length());
    expected_b = (jsonevidence.find(SIMULATED_TYPE_TAG) == std::string::npos ? false : true); 
    LOG_DEBUG("json %s tag %s simulated %d result %d expected %d\n", jsonevidence.c_str(), SIMULATED_TYPE_TAG, jsonevidence.find(SIMULATED_TYPE_TAG) == std::string::npos, b, expected_b);
    COND2LOGERR(b != expected_b, "evidence with bad statement succeeded");

    // this test succeeds for simulated attestations, and fails for real ones
    // test with wrong code id
    b = verify_evidence((uint8_t*)jsonevidence.c_str(), jsonevidence.length(),
        (uint8_t*)expected_statement.c_str(), expected_statement.length(),
        (uint8_t*)wrong_expected_code_id.c_str(), wrong_expected_code_id.length());
    expected_b = (jsonevidence.find(SIMULATED_TYPE_TAG) == std::string::npos ? false : true); 
    COND2LOGERR(b != expected_b, "evidence with bad code id succeeded");

    LOG_INFO("Test Successful\n");
    return true;

err:
    return false;
}
