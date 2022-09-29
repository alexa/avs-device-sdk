/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <string.h>

#include <acsdk/Properties/private/Asn1Helper.h>
#include <acsdk/Properties/private/Asn1Types.h>
#include <acsdk/Properties/private/Logging.h>

/**
 * @brief Macro for cutting off OpenSSL features introduced before 1.1.0 release.
 * @private
 * @ingroup Lib_acsdkProperties
 */
#define OPENSSL_VERSION_NUMBER_1_1_0 0x10100000L

namespace alexaClientSDK {
namespace properties {

/// String to identify log entries originating from this file.
/// @private
#define TAG "Asn1Helper"

/// OK status code for some OpenSSL operations.
/// @private
static constexpr int OPENSSL_OK = 1;

#if OPENSSL_VERSION_NUMBER < OPENSSL_VERSION_NUMBER_1_1_0
/// ERROR status code for some OpenSSL operations.
/// @private
static constexpr int OPENSSL_ERROR = -1;
#endif

bool Asn1Helper::setOptInt(ASN1_INTEGER*& asn1Integer, int64_t value, int64_t defaultValue) noexcept {
    if (value == defaultValue) {
        // If we set optional value to default, we actually need to remove item from DER output.
        if (asn1Integer) {
            ASN1_INTEGER_free(asn1Integer);
            asn1Integer = nullptr;
        }
        return true;
    } else {
        if (!asn1Integer && !(asn1Integer = ASN1_INTEGER_new())) {
            // Failed to allocate memory for ASN.1 integer type.
            ACSDK_ERROR(LX("setOptIntFailed").m("newIntegerFailed"));
            return false;
        } else {
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
            return OPENSSL_OK == ASN1_INTEGER_set_int64(asn1Integer, value);
#else
            if (value <= LONG_MAX && value >= LONG_MIN) {
                return OPENSSL_OK == ASN1_INTEGER_set(asn1Integer, (long)value);
            } else {
                return false;
            }
#endif
        }
    }
}

bool Asn1Helper::getOptInt(ASN1_INTEGER*& asn1Integer, int64_t& value, int64_t defaultValue) noexcept {
    if (!asn1Integer) {
        value = defaultValue;
        return true;
    } else {
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
        return OPENSSL_OK == ASN1_INTEGER_get_int64(&value, asn1Integer);
#else
        long tmp = ASN1_INTEGER_get(asn1Integer);
        if (OPENSSL_ERROR != tmp) {
            value = tmp;
            return true;
        } else {
            return false;
        }
#endif
    }
}

bool Asn1Helper::setStr(ASN1_UTF8STRING*& asn1String, const std::string& value) noexcept {
    if (!asn1String && !(asn1String = ASN1_UTF8STRING_new())) {
        ACSDK_ERROR(LX("setStrFailed").m("newStringFailed"));
        return false;
    } else {
        return OPENSSL_OK == ASN1_STRING_set(asn1String, value.c_str(), static_cast<int>(value.size()));
    }
}

bool Asn1Helper::getStr(ASN1_UTF8STRING*& asn1String, std::string& value) noexcept {
    if (!asn1String) {
        return false;
    } else {
        int size = ASN1_STRING_length(asn1String);
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
        const unsigned char* data = ASN1_STRING_get0_data(asn1String);
#else
        const unsigned char* data = ASN1_STRING_data(asn1String);
#endif
        value.assign((const char*)data, size);
        return true;
    }
}

bool Asn1Helper::setData(ASN1_OCTET_STRING*& asn1String, const Bytes& value) noexcept {
    if (!asn1String && !(asn1String = ASN1_OCTET_STRING_new())) {
        ACSDK_ERROR(LX("setDataFailed").m("newOctetStringFailed"));
        return false;
    } else {
        return OPENSSL_OK == ASN1_OCTET_STRING_set(asn1String, value.data(), static_cast<int>(value.size()));
    }
}

bool Asn1Helper::getData(ASN1_OCTET_STRING*& asn1String, Bytes& value) noexcept {
    if (!asn1String) {
        return false;
    } else {
        int size = ASN1_STRING_length(asn1String);
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
        const unsigned char* data = ASN1_STRING_get0_data(asn1String);
#else
        const unsigned char* data = ASN1_STRING_data(asn1String);
#endif
        value.resize(size);
        memcpy(value.data(), data, size);
        return true;
    }
}

bool Asn1Helper::convertAlgTypeToAsn1(AlgorithmType type, int64_t& asn1Type) noexcept {
    switch (type) {
        case AlgorithmType::AES_256_GCM:
            asn1Type = ACSDK_CIP_ALG_AES_256_GCM;
            break;
        default:
            ACSDK_ERROR(LX("convertAlgTypeToAsn1Failed").d("type", type));
            return false;
    }
    return true;
}

bool Asn1Helper::convertAlgTypeFromAsn1(int64_t asn1Type, AlgorithmType& type) noexcept {
    switch (asn1Type) {
        case ACSDK_CIP_ALG_AES_256_GCM:
            type = AlgorithmType::AES_256_GCM;
            break;
        default:
            ACSDK_ERROR(LX("convertAlgTypeFromAsn1Failed").d("asn1Type", asn1Type));
            return false;
    }
    return true;
}

bool Asn1Helper::convertDigTypeToAsn1(DigestType type, int64_t& asn1Type) noexcept {
    switch (type) {
        case DigestType::SHA_256:
            asn1Type = ACSDK_DIG_ALG_SHA_256;
            break;
        default:
            ACSDK_ERROR(LX("convertDigTypeToAsn1Failed").d("type", type));
            return false;
    }
    return true;
}

bool Asn1Helper::convertDigTypeFromAsn1(int64_t asn1Type, DigestType& type) noexcept {
    switch (asn1Type) {
        case ACSDK_DIG_ALG_SHA_256:
            type = DigestType::SHA_256;
            break;
        default:
            ACSDK_ERROR(LX("convertDigTypeFromAsn1Failed").d("asn1Type", asn1Type));
            return false;
    }
    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
