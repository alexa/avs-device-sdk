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

#include <acsdk/Properties/private/DataPropertyCodec.h>
#include <acsdk/Properties/private/DataPropertyCodecState.h>
#include <acsdk/Properties/private/Logging.h>

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "DataPropertyCodec"

/// @private
static constexpr DigestType DEFAULT_DIGEST_TYPE = DigestType::SHA_256;

bool DataPropertyCodec::encode(
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const IV& dataIV,
    const DataBlock& dataCiphertext,
    const Tag& dataTag,
    DataBlock& derEncoded) noexcept {
    if (!cryptoFactory) {
        ACSDK_ERROR(LX("encodeFailed").m("cryptoFactoryNull"));
        return false;
    }

    const DigestType digestType = DEFAULT_DIGEST_TYPE;
    auto digest = cryptoFactory->createDigest(digestType);
    DataBlock digestData;
    DataBlock encodedInfo;
    DataPropertyCodecState codecState;

    if (!digest) {
        ACSDK_ERROR(LX("encodeFailed").m("digestCreateFailed"));
        return false;
    }

    if (!codecState.prepareForEncode()) {
        ACSDK_ERROR(LX("encodeFailed").m("encodePrepareFailed"));
        return false;
    }

    if (!codecState.setVersion(ACSDK_DATA_KEY_VER_V1)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "version"));
        return false;
    }

    if (!codecState.setDataIV(dataIV)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "dataIV"));
        return false;
    }

    if (!codecState.setDataCiphertext(dataCiphertext)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "dataCiphertext"));
        return false;
    }

    if (!codecState.setDataTag(dataTag)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "dataTag"));
        return false;
    }

    if (!codecState.encodeEncInfo(encodedInfo)) {
        ACSDK_ERROR(LX("encodeFailed").m("encodeInfoFailed"));
        return false;
    }

    if (!digest->process(encodedInfo)) {
        ACSDK_ERROR(LX("encodeFailed").m("digestProcessFailed"));
        return false;
    }

    if (!digest->finalize(digestData)) {
        ACSDK_ERROR(LX("encodeFailed").m("digestFinalizeFailed"));
        return false;
    }

    if (!codecState.setDigestType(digestType)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "digestType"));
        return false;
    }

    if (!codecState.setDigest(digestData)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("propertyName", "digest"));
        return false;
    }

    if (!codecState.encode(derEncoded)) {
        ACSDK_ERROR(LX("encodeFailed").m("finalEncodeFailed"));
        return false;
    }

    return true;
}

bool DataPropertyCodec::decode(
    const std::shared_ptr<CryptoFactoryInterface>& cryptoFactory,
    const std::vector<uint8_t>& derEncoded,
    IV& dataIV,
    DataBlock& dataCiphertext,
    Tag& dataTag,
    DataBlock& digestDecoded,
    DataBlock& digestActual) noexcept {
    if (!cryptoFactory) {
        ACSDK_ERROR(LX("decodeFailed").m("cryptoFactoryNull"));
        return false;
    }

    DataPropertyCodecState codecState;

    int64_t version = 0;
    DigestType digestType = DEFAULT_DIGEST_TYPE;
    std::unique_ptr<DigestInterface> digest;
    DataBlock encoded;

    if (!codecState.decode(derEncoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("initialDecodeFailed"));
        return false;
    }

    if (!codecState.getVersion(version)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertyGetFailed").d("propertyName", "version"));
        return false;
    }

    if (ACSDK_DATA_VER_V1 != version) {
        ACSDK_ERROR(LX("decodeFailed").m("versionError"));
        return false;
    }

    if (!codecState.getDataIV(dataIV)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("propertyName", "dataIV"));
        return false;
    }

    if (!codecState.getDataCiphertext(dataCiphertext)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("propertyName", "dataCiphertext"));
        return false;
    }

    if (!codecState.getDataTag(dataTag)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("propertyName", "dataTag"));
        return false;
    }

    if (!codecState.getDigest(digestDecoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("propertyName", "digest"));
        return false;
    }

    if (!codecState.getDigestType(digestType)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("propertyName", "digestType"));
        return false;
    }

    digest = cryptoFactory->createDigest(digestType);
    if (!digest) {
        ACSDK_ERROR(LX("decodeFailed").m("createDigestFailed"));
        return false;
    }

    if (!codecState.encodeEncInfo(encoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("encodeInfoFailed"));
        return false;
    }

    if (!digest->process(encoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("digestProcessFailed"));
        return false;
    }

    if (!digest->finalize(digestActual)) {
        ACSDK_ERROR(LX("decodeFailed").m("digestFinalizeFailed"));
        return false;
    }

    return true;
}

}  // namespace properties
}  // namespace alexaClientSDK
