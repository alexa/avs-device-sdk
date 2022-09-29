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
#include <acsdk/Properties/private/EncryptionKeyPropertyCodec.h>
#include <acsdk/Properties/private/EncryptionKeyPropertyCodecState.h>
#include <acsdk/Properties/private/Logging.h>

/// String to identify log entries originating from this file.
/// @private
#define TAG "EncryptionKeyPropertyCodec"

namespace alexaClientSDK {
namespace properties {

using namespace ::alexaClientSDK::cryptoInterfaces;

/// Default digest type used when creating new digests.
/// @private
static constexpr DigestType DEFAULT_DIGEST_TYPE = DigestType::SHA_256;

bool EncryptionKeyPropertyCodec::encode(
    const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const std::string& mainKeyAlias,
    const KeyChecksum& mainKeyChecksum,
    alexaClientSDK::cryptoInterfaces::AlgorithmType dataKeyAlgorithm,
    const IV& dataKeyIV,
    const DataBlock& dataKeyCiphertext,
    const Tag& dataKeyTag,
    alexaClientSDK::cryptoInterfaces::AlgorithmType dataAlgorithm,
    Bytes& derEncoded) noexcept {
    DigestType digestType = DEFAULT_DIGEST_TYPE;

    if (!cryptoFactory) {
        ACSDK_ERROR(LX("encodeFailed").m("cryptoFactoryNull"));
        return false;
    }

    auto digest = cryptoFactory->createDigest(digestType);
    if (!digest) {
        ACSDK_ERROR(LX("encodeFailed").m("digestCreateFailed"));
        return false;
    }

    EncryptionKeyPropertyCodecState helper;
    DataBlock digestData;
    DataBlock encodedInfo;

    if (!helper.prepareForEncoding()) {
        ACSDK_ERROR(LX("encodeFailed").m("encodingPrepareFailed"));
        return false;
    }

    if (!helper.setVersion(ACSDK_DATA_KEY_VER_V1)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "version"));
        return false;
    }

    if (!helper.setMainKeyAlias(mainKeyAlias)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "mainKeyAlias"));
        return false;
    }

    if (!helper.setMainKeyChecksum(mainKeyChecksum)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "mainKeyChecksum"));
        return false;
    }

    if (!helper.setDataKeyAlgorithm(dataKeyAlgorithm)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "dataKeyAlgorithm"));
        return false;
    }

    if (!helper.setDataKeyIV(dataKeyIV)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "dataKeyIV"));
        return false;
    }

    if (!helper.setDataKeyCiphertext(dataKeyCiphertext)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "dataKeyCiphertext"));
        return false;
    }

    if (!helper.setDataKeyTag(dataKeyTag)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "dataKeyTag"));
        return false;
    }

    if (!helper.setDataAlgorithm(dataAlgorithm)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "dataAlgorithm"));
        return false;
    }

    if (!helper.encodeEncInfo(encodedInfo)) {
        ACSDK_ERROR(LX("encodeFailed").m("infoEncodingFailed"));
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

    if (!helper.setDigestType(digestType)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "digestType"));
        return false;
    }

    if (!helper.setDigest(digestData)) {
        ACSDK_ERROR(LX("encodeFailed").m("propertySetFailed").d("property", "digestData"));
        return false;
    }

    if (!helper.encode(derEncoded)) {
        ACSDK_ERROR(LX("encodeFailed").m("finalEncodeFailed"));
        return false;
    }

    return true;
}

bool EncryptionKeyPropertyCodec::decode(
    const std::shared_ptr<cryptoInterfaces::CryptoFactoryInterface>& cryptoFactory,
    const Bytes& derEncoded,
    std::string& mainKeyAlias,
    KeyChecksum& mainKeyChecksum,
    alexaClientSDK::cryptoInterfaces::AlgorithmType& dataKeyAlgorithm,
    IV& dataKeyIV,
    DataBlock& dataKeyCiphertext,
    Tag& dataKeyTag,
    alexaClientSDK::cryptoInterfaces::AlgorithmType& dataAlgorithm,
    DataBlock& digestDecoded,
    DataBlock& digestActual) noexcept {
    if (!cryptoFactory) {
        ACSDK_ERROR(LX("decodeFailed").m("cryptoFactoryNull"));
        return false;
    }

    EncryptionKeyPropertyCodecState codecState;
    DigestType digestType = DigestType::SHA_256;

    if (!codecState.decode(derEncoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("initialDecodeFailed"));
        return false;
    }
    Bytes encoded;
    int64_t version = 0;

    if (!codecState.getVersion(version)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "version"));
        return false;
    }

    if (ACSDK_DATA_KEY_VER_V1 != version) {
        ACSDK_ERROR(LX("decodeFailed").m("versionError"));
        return false;
    }

    if (!codecState.getMainKeyAlias(mainKeyAlias)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "mainKeyAlias"));
        return false;
    }

    if (!codecState.getMainKeyChecksum(mainKeyChecksum)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "mainKeyChecksum"));
        return false;
    }

    if (!codecState.getDataKeyAlgorithm(dataKeyAlgorithm)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "dataKeyAlgorithm"));
        return false;
    }

    if (!codecState.getDataKeyIV(dataKeyIV)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "dataKeyIV"));
        return false;
    }

    if (!codecState.getDataKeyCiphertext(dataKeyCiphertext)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "dataKeyCiphertext"));
        return false;
    }

    if (!codecState.getDataKeyTag(dataKeyTag)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "dataKeyTag"));
        return false;
    }

    if (!codecState.getDataAlgorithm(dataAlgorithm)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "dataAlgorithm"));
        return false;
    }

    if (!codecState.getDigest(digestDecoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "digest"));
        return false;
    }

    if (!codecState.getDigestType(digestType)) {
        ACSDK_ERROR(LX("decodeFailed").m("propertyGetFailed").d("property", "digestType"));
        return false;
    }

    if (!codecState.encodeEncInfo(encoded)) {
        ACSDK_ERROR(LX("decodeFailed").m("encodeInfoFailed"));
        return false;
    }

    auto digest = cryptoFactory->createDigest(digestType);
    if (!digest) {
        ACSDK_ERROR(LX("decodeFailed").m("digestInitFailed"));
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
