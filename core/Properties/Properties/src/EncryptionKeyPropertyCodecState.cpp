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

#include <stdlib.h>

#include <acsdk/Properties/private/EncryptionKeyPropertyCodec.h>
#include <acsdk/Properties/private/EncryptionKeyPropertyCodecState.h>
#include <acsdk/Properties/private/Asn1Helper.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::cryptoInterfaces;

EncryptionKeyPropertyCodecState::EncryptionKeyPropertyCodecState() noexcept {
    m_asn1Data = nullptr;
}

EncryptionKeyPropertyCodecState::~EncryptionKeyPropertyCodecState() noexcept {
    ACSDK_ENC_PROP_free(m_asn1Data);
    m_asn1Data = nullptr;
}

bool EncryptionKeyPropertyCodecState::prepareForEncoding() noexcept {
    if (!m_asn1Data && !(m_asn1Data = ACSDK_ENC_PROP_new())) {
        return false;
    }
    if (!m_asn1Data->encryptionInfo && !(m_asn1Data->encryptionInfo = ACSDK_ENC_INFO_new())) {
        return false;
    }
    return true;
}

bool EncryptionKeyPropertyCodecState::setVersion(int64_t value) noexcept {
    return m_asn1Data->encryptionInfo &&
           Asn1Helper::setOptInt(m_asn1Data->encryptionInfo->version, value, ACSDK_ENC_INFO::DEF_VER);
}

bool EncryptionKeyPropertyCodecState::getVersion(int64_t& value) noexcept {
    return m_asn1Data->encryptionInfo &&
           Asn1Helper::getOptInt(m_asn1Data->encryptionInfo->version, value, ACSDK_ENC_INFO::DEF_VER);
}

bool EncryptionKeyPropertyCodecState::setMainKeyAlias(const std::string& value) noexcept {
    return Asn1Helper::setStr(m_asn1Data->encryptionInfo->mainKeyAlias, value);
}

bool EncryptionKeyPropertyCodecState::getMainKeyAlias(std::string& value) noexcept {
    return Asn1Helper::getStr(m_asn1Data->encryptionInfo->mainKeyAlias, value);
}

bool EncryptionKeyPropertyCodecState::setMainKeyChecksum(const std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::setData(m_asn1Data->encryptionInfo->mainKeyChecksum, value);
}

bool EncryptionKeyPropertyCodecState::getMainKeyChecksum(std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::getData(m_asn1Data->encryptionInfo->mainKeyChecksum, value);
}

bool EncryptionKeyPropertyCodecState::setDataKeyAlgorithm(AlgorithmType value) noexcept {
    int64_t asn1Value;
    return Asn1Helper::convertAlgTypeToAsn1(value, asn1Value) &&
           Asn1Helper::setOptInt(
               m_asn1Data->encryptionInfo->dataKeyAlgorithm, asn1Value, ACSDK_ENC_INFO::DEF_DATA_KEY_ALG);
}

bool EncryptionKeyPropertyCodecState::getDataKeyAlgorithm(AlgorithmType& value) noexcept {
    int64_t asn1Value;
    return Asn1Helper::getOptInt(
               m_asn1Data->encryptionInfo->dataKeyAlgorithm, asn1Value, ACSDK_ENC_INFO::DEF_DATA_KEY_ALG) &&
           Asn1Helper::convertAlgTypeFromAsn1(asn1Value, value);
}

bool EncryptionKeyPropertyCodecState::setDataKeyIV(const std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::setData(m_asn1Data->encryptionInfo->dataKeyIV, value);
}

bool EncryptionKeyPropertyCodecState::getDataKeyIV(std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::getData(m_asn1Data->encryptionInfo->dataKeyIV, value);
}

bool EncryptionKeyPropertyCodecState::setDataKeyCiphertext(const std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::setData(m_asn1Data->encryptionInfo->dataKeyCiphertext, value);
}

bool EncryptionKeyPropertyCodecState::getDataKeyCiphertext(std::vector<uint8_t>& value) noexcept {
    return Asn1Helper::getData(m_asn1Data->encryptionInfo->dataKeyCiphertext, value);
}

bool EncryptionKeyPropertyCodecState::setDataKeyTag(const Tag& dataKeyTag) noexcept {
    return Asn1Helper::setData(m_asn1Data->encryptionInfo->dataKeyTag, dataKeyTag);
}

bool EncryptionKeyPropertyCodecState::getDataKeyTag(std::vector<uint8_t>& dataKeyTag) noexcept {
    return Asn1Helper::getData(m_asn1Data->encryptionInfo->dataKeyTag, dataKeyTag);
}

bool EncryptionKeyPropertyCodecState::setDataAlgorithm(AlgorithmType value) noexcept {
    int64_t asn1Value;
    return Asn1Helper::convertAlgTypeToAsn1(value, asn1Value) &&
           Asn1Helper::setOptInt(m_asn1Data->encryptionInfo->dataAlgorithm, asn1Value, ACSDK_ENC_INFO::DEF_DATA_ALG);
}

bool EncryptionKeyPropertyCodecState::getDataAlgorithm(AlgorithmType& value) noexcept {
    int64_t asn1Value;
    return Asn1Helper::getOptInt(m_asn1Data->encryptionInfo->dataAlgorithm, asn1Value, ACSDK_ENC_INFO::DEF_DATA_ALG) &&
           Asn1Helper::convertAlgTypeFromAsn1(asn1Value, value);
}

bool EncryptionKeyPropertyCodecState::setDigestType(DigestType type) noexcept {
    int64_t asn1Type;
    return Asn1Helper::convertDigTypeToAsn1(type, asn1Type) &&
           Asn1Helper::setOptInt(m_asn1Data->digestAlgorithm, asn1Type, ACSDK_ENC_PROP::DEF_DIG_ALG);
}

bool EncryptionKeyPropertyCodecState::getDigestType(DigestType& type) noexcept {
    int64_t asn1Type;
    return Asn1Helper::getOptInt(m_asn1Data->digestAlgorithm, asn1Type, ACSDK_ENC_PROP::DEF_DIG_ALG) &&
           Asn1Helper::convertDigTypeFromAsn1(asn1Type, type);
}

bool EncryptionKeyPropertyCodecState::setDigest(const std::vector<uint8_t>& digest) noexcept {
    return Asn1Helper::setData(m_asn1Data->digest, digest);
}

bool EncryptionKeyPropertyCodecState::getDigest(std::vector<uint8_t>& digest) noexcept {
    return Asn1Helper::getData(m_asn1Data->digest, digest);
}

bool EncryptionKeyPropertyCodecState::encodeEncInfo(std::vector<uint8_t>& der) noexcept {
    if (!m_asn1Data->encryptionInfo) {
        return false;
    }

    int res = i2d_ACSDK_ENC_INFO(m_asn1Data->encryptionInfo, nullptr);
    if (res <= 0) {
        return false;
    }

    der.resize(res);
    unsigned char* data = der.data();
    res = i2d_ACSDK_ENC_INFO(m_asn1Data->encryptionInfo, &data);
    if (res <= 0) {
        return false;
    }
    return true;
}

bool EncryptionKeyPropertyCodecState::encode(std::vector<uint8_t>& der) noexcept {
    int res = i2d_ACSDK_ENC_PROP(m_asn1Data, nullptr);
    if (res <= 0) {
        return false;
    }

    der.resize(res);
    unsigned char* data = der.data();
    res = i2d_ACSDK_ENC_PROP(m_asn1Data, &data);

    if (res <= 0) {
        return false;
    }
    return true;
}

bool EncryptionKeyPropertyCodecState::decode(const std::vector<uint8_t>& der) noexcept {
    if (m_asn1Data) {
        ACSDK_ENC_PROP_free(m_asn1Data);
        m_asn1Data = nullptr;
    }
    const unsigned char* data = der.data();
    m_asn1Data = d2i_ACSDK_ENC_PROP(nullptr, &data, static_cast<long>(der.size()));
    if (!m_asn1Data) {
        return false;
    }

    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
