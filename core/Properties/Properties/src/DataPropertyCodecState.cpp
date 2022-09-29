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

#include <acsdk/Properties/private/Asn1Helper.h>
#include <acsdk/Properties/private/DataPropertyCodecState.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "DataPropertyCodecState"

DataPropertyCodecState::DataPropertyCodecState() noexcept {
    m_asn1Data = nullptr;
}

DataPropertyCodecState::~DataPropertyCodecState() noexcept {
    ACSDK_DATA_PROP_free(m_asn1Data);
    m_asn1Data = nullptr;
}

bool DataPropertyCodecState::prepareForEncode() noexcept {
    if (!m_asn1Data && !(m_asn1Data = ACSDK_DATA_PROP_new())) {
        ACSDK_ERROR(LX("prepareForEncodeFailed").m("newDataPropFailed"));
        return false;
    }
    if (!m_asn1Data->dataInfo && !(m_asn1Data->dataInfo = ACSDK_DATA_INFO_new())) {
        ACSDK_ERROR(LX("prepareForEncodeFailed").m("newDataInfoFailed"));
        return false;
    }
    return true;
}

bool DataPropertyCodecState::setVersion(int64_t value) noexcept {
    return m_asn1Data->dataInfo && Asn1Helper::setOptInt(m_asn1Data->dataInfo->version, value, ACSDK_ENC_INFO::DEF_VER);
}

bool DataPropertyCodecState::getVersion(int64_t& value) noexcept {
    return m_asn1Data->dataInfo && Asn1Helper::getOptInt(m_asn1Data->dataInfo->version, value, ACSDK_ENC_INFO::DEF_VER);
}

bool DataPropertyCodecState::setDataIV(const IV& value) noexcept {
    return Asn1Helper::setData(m_asn1Data->dataInfo->dataIV, value);
}

bool DataPropertyCodecState::getDataIV(IV& value) noexcept {
    return Asn1Helper::getData(m_asn1Data->dataInfo->dataIV, value);
}

bool DataPropertyCodecState::setDataCiphertext(const DataBlock& value) noexcept {
    return Asn1Helper::setData(m_asn1Data->dataInfo->dataCiphertext, value);
}

bool DataPropertyCodecState::getDataCiphertext(DataBlock& value) noexcept {
    return Asn1Helper::getData(m_asn1Data->dataInfo->dataCiphertext, value);
}

bool DataPropertyCodecState::setDataTag(const Tag& dataTag) noexcept {
    return Asn1Helper::setData(m_asn1Data->dataInfo->dataTag, dataTag);
}

bool DataPropertyCodecState::getDataTag(Tag& dataTag) noexcept {
    return Asn1Helper::getData(m_asn1Data->dataInfo->dataTag, dataTag);
}

bool DataPropertyCodecState::setDigestType(DigestType type) noexcept {
    int64_t asn1Type;
    return Asn1Helper::convertDigTypeToAsn1(type, asn1Type) &&
           Asn1Helper::setOptInt(m_asn1Data->digestAlgorithm, asn1Type, ACSDK_ENC_PROP::DEF_DIG_ALG);
}

bool DataPropertyCodecState::getDigestType(DigestType& type) noexcept {
    int64_t asn1Type;
    return Asn1Helper::getOptInt(m_asn1Data->digestAlgorithm, asn1Type, ACSDK_ENC_PROP::DEF_DIG_ALG) &&
           Asn1Helper::convertDigTypeFromAsn1(asn1Type, type);
}

bool DataPropertyCodecState::setDigest(const DataBlock& digest) noexcept {
    return Asn1Helper::setData(m_asn1Data->digest, digest);
}

bool DataPropertyCodecState::getDigest(DataBlock& digest) noexcept {
    return Asn1Helper::getData(m_asn1Data->digest, digest);
}

bool DataPropertyCodecState::encodeEncInfo(DataBlock& der) noexcept {
    if (!m_asn1Data->dataInfo) {
        ACSDK_ERROR(LX("encodeEncInfoFailed").m("nullDataInfo"));
        return false;
    }

    int res = i2d_ACSDK_DATA_INFO(m_asn1Data->dataInfo, nullptr);
    if (res <= 0) {
        ACSDK_ERROR(LX("encodeEncInfoFailed").m("derEncodingEstimateFailed"));
        return false;
    }

    der.resize(res);
    unsigned char* data = der.data();
    res = i2d_ACSDK_DATA_INFO(m_asn1Data->dataInfo, &data);
    if (res <= 0) {
        ACSDK_ERROR(LX("encodeEncInfoFailed").m("derEncodingFailed"));
        return false;
    }
    return true;
}

bool DataPropertyCodecState::encode(DataBlock& der) noexcept {
    int res = i2d_ACSDK_DATA_PROP(m_asn1Data, nullptr);
    if (res <= 0) {
        ACSDK_ERROR(LX("encodeEncInfoFailed").m("sizeEstimateFailed"));
        return false;
    }

    der.resize(res);
    unsigned char* data = der.data();
    res = i2d_ACSDK_DATA_PROP(m_asn1Data, &data);

    if (res <= 0) {
        ACSDK_ERROR(LX("encodeEncInfoFailed").m("derEncodingFailed"));
        return false;
    }
    return true;
}

bool DataPropertyCodecState::decode(const DataBlock& der) noexcept {
    if (m_asn1Data) {
        ACSDK_DEBUG9(LX("decodeReleasingData"));
        ACSDK_DATA_PROP_free(m_asn1Data);
        m_asn1Data = nullptr;
    }
    const unsigned char* data = der.data();
    m_asn1Data = d2i_ACSDK_DATA_PROP(nullptr, &data, static_cast<long>(der.size()));
    if (!m_asn1Data || !m_asn1Data->dataInfo) {
        ACSDK_ERROR(LX("decodeFailed").m("derDecodeFailed"));
        return false;
    }

    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
