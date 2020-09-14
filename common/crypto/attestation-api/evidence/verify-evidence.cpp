/*
 * Copyright 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "verify-evidence.h"
#include "attestation_tags.h"
#include <string>
#include <string.h>
#include <vector>
#include "base64.h"
#include "crypto.h"
#include "parson.h"
#include "pdo/common/jsonvalue.h"
// TODO remove all logs
//
#define PDO_DEBUG_BUILD 1
#include "log.h"
#include "pdo_error.h"

//#include "logging.h"
#define LOG_DEBUG(fmt, ...) SAFE_LOG(PDO_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) SAFE_LOG(PDO_LOG_INFO, fmt, ##__VA_ARGS__)

#include "error.h"

bool unwrap_ias_evidence(const std::string& evidence_str,
    std::string& ias_signature,
    std::string& ias_certificates,
    std::string& ias_report)
{
    JsonValue root_value = json_parse_string(evidence_str.c_str());
    JSON_Object* root_object = json_value_get_object(root_value);
    COND2ERR(root_object == NULL);

    ias_signature = json_object_get_string(root_object, "iasSignature");
    COND2ERR(ias_signature.length() == 0);
    LOG_DEBUG("signature: %s\n", ias_signature.c_str());

    ias_certificates = json_object_get_string(root_object, "iasCertificates");
    COND2ERR(ias_certificates.length() == 0);
    LOG_DEBUG("certificates: %s\n", ias_certificates.c_str());

    ias_report = json_object_get_string(root_object, "iasReport");
    COND2ERR(ias_report.length() == 0);
    LOG_DEBUG("report: %s\n", ias_report.c_str());

    return true;

err:
    LOG_DEBUG("ias evidence: %s\n", evidence_str.c_str());
    return false;
}

void replace_all_substrings(
    std::string& s, const std::string& substring, const std::string& replace_with)
{
    size_t pos = 0;
    while (1)
    {
        pos = s.find(substring, pos);
        if (pos == std::string::npos)
            break;

        s.replace(pos, substring.length(), replace_with);
    }
}

void url_decode_ias_certificate(std::string& s)
{
    replace_all_substrings(s, "%20", " ");
    replace_all_substrings(s, "%0A", "\n");
    replace_all_substrings(s, "%2B", "+");
    replace_all_substrings(s, "%3D", "=");
}

bool split_certificates(
    std::string& ias_certificates, std::vector<std::string>& ias_certificate_vector)
{
    // ias certificates should have 2 certificates "-----BEGIN CERTIFICATE----- [...] -----END
    // CERTIFICATE-----\n"
    std::string cert_start("-----BEGIN CERTIFICATE-----");
    std::string cert_end("-----END CERTIFICATE-----\n");
    size_t cur = 0, start = 0, end = 0;

    ias_certificate_vector.clear();

    url_decode_ias_certificate(ias_certificates);

    while (1)
    {
        start = ias_certificates.find(cert_start, cur);
        if (start == std::string::npos)
        {
            break;
        }

        end = ias_certificates.find(cert_end, cur);
        if (end == std::string::npos)
        {
            break;
        }
        end += cert_end.length();

        ias_certificate_vector.push_back(ias_certificates.substr(start, end));
        cur = end;
    }

    COND2ERR(ias_certificate_vector.size() != 2);

    return true;

err:
    return false;
}

bool extract_hex_from_report(
    const std::string& ias_report, size_t offset, size_t size, std::string& hex)
{
    std::string b64quote;
    ByteArray bin_quote;
    sgx_report_body_t* rb;
    ByteArray ba;

    JsonValue root_value = json_parse_string(ias_report.c_str());
    JSON_Object* root_object = json_value_get_object(root_value);
    COND2ERR(root_object == NULL);

    b64quote = json_object_get_string(root_object, "isvEnclaveQuoteBody");
    COND2ERR(b64quote.length() == 0);
    LOG_DEBUG("b64quote: %s\n", b64quote.c_str());

    bin_quote = Base64EncodedStringToByteArray(b64quote);
    LOG_DEBUG("bin quote size %d, struct quote size %d\n", bin_quote.size(), sizeof(sgx_quote_t));
    COND2ERR(bin_quote.size() != offsetof(sgx_quote_t, signature_len));
    ba = ByteArray(bin_quote.data() + offset, bin_quote.data() + offset + size);
    hex = ByteArrayToHexEncodedString(ba);

    return true;

err:
    return false;
}

bool verify_ias_evidence(ByteArray& evidence, ByteArray& expected_statement, ByteArray& expected_code_id)
{
    std::string evidence_str((char*)evidence.data(), evidence.size());
    std::string expected_hex_id((char*)expected_code_id.data(), expected_code_id.size());

    std::string ias_signature, ias_certificates, ias_report;
    std::vector<std::string> ias_certificate_vector;

    // get evidence data
    COND2ERR(false ==
                 unwrap_ias_evidence(evidence_str, ias_signature, ias_certificates, ias_report));

    // split certs
    COND2ERR(false == split_certificates(ias_certificates, ias_certificate_vector));

    {
        // verify report status
        const int group_out_of_date_ok = 1;
        COND2ERR(VERIFY_SUCCESS != verify_enclave_quote_status(ias_report.c_str(),
                                   ias_report.length(), group_out_of_date_ok));
    }

    {
        // check root cert
        const int root_certificate_index = 1;
        COND2ERR(VERIFY_SUCCESS != verify_ias_certificate_chain(
                                       ias_certificate_vector[root_certificate_index].c_str()));
    }

    {
        // check signing cert
        const int signing_certificate_index = 0;
        COND2ERR(VERIFY_SUCCESS != verify_ias_certificate_chain(
                                       ias_certificate_vector[signing_certificate_index].c_str()));

        // check signature
        COND2ERR(VERIFY_SUCCESS != verify_ias_report_signature(
                                       ias_certificate_vector[signing_certificate_index].c_str(),
                                       ias_report.c_str(), ias_report.length(),
                                       (char*)ias_signature.c_str(), ias_signature.length()));
    }

    {
        // check code id
        std::string hex_id;
        COND2ERR(false ==
                 extract_hex_from_report(ias_report,
                     offsetof(sgx_quote_t, report_body) + offsetof(sgx_report_body_t, mr_enclave),
                     sizeof(sgx_measurement_t), hex_id));
        LOG_DEBUG("hex id(%u): %s\n", hex_id.length(), hex_id.c_str());
        LOG_DEBUG("expected hex id(%u): %s\n", expected_hex_id.length(), expected_hex_id.c_str());
        COND2ERR(0 != hex_id.compare(expected_hex_id));
    }

    {
        // check report data
        std::string hex_report_data, expected_hex_report_data_str;
        COND2ERR(false ==
                 extract_hex_from_report(ias_report,
                     offsetof(sgx_quote_t, report_body) + offsetof(sgx_report_body_t, report_data),
                     sizeof(sgx_report_data_t), hex_report_data));
        expected_hex_report_data_str = ByteArrayToHexEncodedString(pdo::crypto::ComputeMessageHash(expected_statement));
        expected_hex_report_data_str.append(expected_hex_report_data_str.length(), '0');
        LOG_DEBUG("hex report data: %s\n", hex_report_data.c_str());
        LOG_DEBUG("expected hex report data: %s\n", expected_hex_report_data_str.c_str());
        COND2ERR(0 != hex_report_data.compare(expected_hex_report_data_str));
    }

    return true;

err:
    return false;
}

bool verify_evidence(uint8_t* evidence,
    uint32_t evidence_length,
    uint8_t* expected_statement,
    uint32_t expected_statement_length,
    uint8_t* expected_code_id,
    uint32_t expected_code_id_length)
{
    bool ret = false;
    std::string evidence_str((char*)evidence, evidence_length);
    ByteArray ba_expected_statement(expected_statement, expected_statement + expected_statement_length);
    ByteArray ba_expected_code_id(expected_code_id, expected_code_id + expected_code_id_length);

    LOG_DEBUG("verify evidence of %s\n", evidence_str.c_str());

    JsonValue root_value = json_parse_string(evidence_str.c_str());
    JSON_Object* root_object = json_value_get_object(root_value);

    const char* s = json_object_get_string(root_object, ATTESTATION_TYPE_TAG);
    std::string attestation_type(s?s:"");
    const char* evidence_field = json_object_get_string(root_object, EVIDENCE_TAG);
    COND2LOGERR(root_object == NULL, "invalid input");
    COND2LOGERR(s == NULL, "no attestation type");
    COND2LOGERR(evidence_field == NULL, "no evidence field");

    if(0 == attestation_type.compare(SIMULATED_TYPE_TAG))
    {
        // nothing to check
        ret = true;
    }

    if(0 == attestation_type.compare(EPID_LINKABLE_TYPE_TAG) || 0 == attestation_type.compare(EPID_UNLINKABLE_TYPE_TAG))
    {
        LOG_DEBUG("evidence field (%u): %s",  strlen(evidence_field), evidence_field);
        ByteArray ba_evidence(evidence_field, evidence_field + strlen(evidence_field));
        COND2ERR(false == verify_ias_evidence(ba_evidence, ba_expected_statement, ba_expected_code_id));
        ret = true;
    }

    COND2LOGERR(ret == false, "verify evidence failed");

    LOG_DEBUG("verify evidence success cpp\n");
    return true;

err:
    return false;
}
