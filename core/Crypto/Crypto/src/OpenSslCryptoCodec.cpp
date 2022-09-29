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

#include <acsdk/Crypto/private/Logging.h>
#include <acsdk/Crypto/private/OpenSslCryptoCodec.h>
#include <acsdk/Crypto/private/OpenSslErrorCleanup.h>
#include <acsdk/Crypto/private/OpenSslTypeMapper.h>

namespace alexaClientSDK {
namespace crypto {

using namespace ::alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "OpenSSL::CryptoCodec"

std::unique_ptr<OpenSslCryptoCodec> OpenSslCryptoCodec::createDecoder(AlgorithmType type) noexcept {
    ACSDK_DEBUG9(LX("createDecoder").d("algorithmType", type));
    auto cipher = createCodec(type, CodecType::Decoder);
    if (!cipher) {
        ACSDK_ERROR(LX("createDecoderFailed").d("algorithmType", type));
    }
    return cipher;
}

std::unique_ptr<OpenSslCryptoCodec> OpenSslCryptoCodec::createEncoder(AlgorithmType type) noexcept {
    ACSDK_DEBUG9(LX("createEncoder").d("algorithmType", type));
    auto cipher = createCodec(type, CodecType::Encoder);
    if (!cipher) {
        ACSDK_ERROR(LX("createEncoderFailed").d("algorithmType", type));
    }
    return cipher;
}

std::unique_ptr<OpenSslCryptoCodec> OpenSslCryptoCodec::createCodec(AlgorithmType type, CodecType codecType) noexcept {
    const EVP_CIPHER* cipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(type);

    if (!cipher) {
        return nullptr;
    }

    return std::unique_ptr<OpenSslCryptoCodec>(new OpenSslCryptoCodec(codecType, type));
}

OpenSslCryptoCodec::OpenSslCryptoCodec(CodecType codecType, AlgorithmType algorithmType) noexcept :
        m_codecType{codecType},
        m_algorithmType{algorithmType},
        m_cipherCtx{EVP_CIPHER_CTX_new()},
        m_initDone{false} {
}

OpenSslCryptoCodec::~OpenSslCryptoCodec() noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    EVP_CIPHER_CTX_free(m_cipherCtx);
}

bool OpenSslCryptoCodec::init(const Key& key, const IV& iv) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};

    m_cipher = OpenSslTypeMapper::mapAlgorithmToEvpCipher(m_algorithmType);
    if (!m_cipher) {
        ACSDK_ERROR(LX("initFailed").d("reason", "cipherNull"));
        return false;
    }

    PaddingMode paddingMode = PaddingMode::NoPadding;

    if (!OpenSslTypeMapper::mapAlgorithmToPadding(m_algorithmType, paddingMode)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "badPaddingMode"));
        return false;
    }

    auto cipher_key_length = EVP_CIPHER_key_length(m_cipher);
    auto ivLength = EVP_CIPHER_iv_length(m_cipher);

    m_initDone = false;

    if (iv.size() != (size_t)ivLength) {
        ACSDK_ERROR(LX("initFailed").d("reason", "badIvSize"));
        return false;
    }

    if (key.size() != (size_t)cipher_key_length) {
        ACSDK_ERROR(LX("initFailed").d("reason", "badKeySize"));
        return false;
    }

    EVP_CIPHER_CTX_init(m_cipherCtx);
    if (OPENSSL_OK ==
        EVP_CipherInit_ex(m_cipherCtx, m_cipher, nullptr, &key[0], &iv[0], static_cast<int>(m_codecType))) {
        if (OPENSSL_OK == EVP_CIPHER_CTX_set_padding(m_cipherCtx, static_cast<int>(paddingMode))) {
            m_initDone = true;
        } else {
            ACSDK_ERROR(LX("initFailed").m("failedToSetPadding"));
        }
    } else {
        ACSDK_ERROR(LX("initFailed").d("reason", "cipherInitFailed"));
    }
    if (!m_initDone) {
        EVP_CIPHER_CTX_cleanup(m_cipherCtx);
    }

    return m_initDone;
}

bool OpenSslCryptoCodec::processAAD(
    DataBlock::const_iterator dataInBegin,
    DataBlock::const_iterator dataInEnd) noexcept {
    if (!m_initDone) {
        ACSDK_ERROR(LX("processAADFailed").d("reason", "cipherIsNotInitialized"));
        return false;
    }
    if (dataInBegin > dataInEnd) {
        ACSDK_ERROR(LX("processAADFailed").d("reason", "dataInAfterDataOut"));
        return false;
    }
    if (dataInBegin == dataInEnd) {
        return true;
    }

    if (!isAEADCipher()) {
        ACSDK_ERROR(LX("processAADFailed").d("reason", "notAEAD"));
        return false;
    }

    OpenSslErrorCleanup errorCleanup{TAG};

    int cipherBlockSize = EVP_CIPHER_block_size(m_cipher);
    int outLen = static_cast<int>((dataInEnd - dataInBegin) + cipherBlockSize);

    if (OPENSSL_OK ==
        EVP_CipherUpdate(m_cipherCtx, nullptr, &outLen, &*dataInBegin, static_cast<int>(dataInEnd - dataInBegin))) {
        return true;
    } else {
        ACSDK_ERROR(LX("processAADFailed").d("reason", "cipherUpdateFailed"));
        EVP_CIPHER_CTX_cleanup(m_cipherCtx);
        m_initDone = false;
        return false;
    }
}

bool OpenSslCryptoCodec::processAAD(const DataBlock& dataIn) noexcept {
    return processAAD(dataIn.cbegin(), dataIn.cend());
}

bool OpenSslCryptoCodec::process(
    DataBlock::const_iterator dataInBegin,
    DataBlock::const_iterator dataInEnd,
    DataBlock& dataOut) noexcept {
    if (!m_initDone) {
        ACSDK_ERROR(LX("processFailed").d("reason", "cipherIsNotInitialized"));
        return false;
    }
    if (dataInBegin > dataInEnd) {
        ACSDK_ERROR(LX("processFailed").d("reason", "dataInAfterDataOut"));
        return false;
    }
    if (dataInBegin == dataInEnd) {
        return true;
    }

    OpenSslErrorCleanup errorCleanup{TAG};

    int cipherBlockSize = EVP_CIPHER_block_size(m_cipher);

    int outLen = static_cast<int>((dataInEnd - dataInBegin) + cipherBlockSize);
    size_t index = dataOut.size();
    dataOut.resize(index + outLen);

    if (OPENSSL_OK ==
        EVP_CipherUpdate(
            m_cipherCtx, &dataOut[index], &outLen, &*dataInBegin, static_cast<int>(dataInEnd - dataInBegin))) {
        dataOut.resize(index + outLen);

        return true;
    } else {
        ACSDK_ERROR(LX("processFailed").d("reason", "cipherUpdateFailed"));
        EVP_CIPHER_CTX_cleanup(m_cipherCtx);
        m_initDone = false;
        return false;
    }
}

bool OpenSslCryptoCodec::process(const DataBlock& dataIn, DataBlock& dataOut) noexcept {
    return process(dataIn.cbegin(), dataIn.cend(), dataOut);
}

bool OpenSslCryptoCodec::finalize(DataBlock& dataOut) noexcept {
    if (!m_initDone) {
        ACSDK_ERROR(LX("finalizeFailed").d("reason", "cipherIsNotInitialized"));
        return false;
    }

    OpenSslErrorCleanup errorCleanup{TAG};

    int cipherBlockSize = EVP_CIPHER_block_size(m_cipher);
    size_t index = dataOut.size();
    dataOut.resize(index + cipherBlockSize);
    int outLen = cipherBlockSize;

    int rv = EVP_CipherFinal_ex(m_cipherCtx, (unsigned char*)&dataOut[index], &outLen);
    if (!isAEADCipher() || CodecType::Decoder == m_codecType) {
        m_initDone = false;
        EVP_CIPHER_CTX_cleanup(m_cipherCtx);
    }
    if (OPENSSL_OK != rv) {
        ACSDK_ERROR(LX("finalizeFailed").d("reason", "cipherFinalFailed"));
        return false;
    }

    dataOut.resize(index + outLen);
    return true;
}

bool OpenSslCryptoCodec::getTag(Tag& tag) noexcept {
    if (!m_initDone) {
        ACSDK_ERROR(LX("getTagFailed").d("reason", "cipherIsNotInitialized"));
        return false;
    }
    if (!isAEADCipher()) {
        ACSDK_ERROR(LX("getTagFailed").d("reason", "notAEAD"));
        return false;
    }
    if (CodecType::Encoder != m_codecType) {
        ACSDK_ERROR(LX("getTagFailed").d("reason", "notEncoder"));
        return false;
    }
    size_t tagSize;
    if (!OpenSslTypeMapper::mapAlgorithmToTagSize(m_algorithmType, tagSize)) {
        ACSDK_ERROR(LX("getTagFailed").d("reason", "tagSizeUnknown"));
        return false;
    }
    size_t tagOffset = tag.size();
    tag.resize(tagOffset + tagSize);
    OpenSslErrorCleanup errorCleanup{TAG};

    if (OPENSSL_OK !=
        EVP_CIPHER_CTX_ctrl(m_cipherCtx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag.size()), tag.data() + tagOffset)) {
        ACSDK_ERROR(LX("getTagFailed").d("reason", "sslError"));
        return false;
    }

    m_initDone = false;
    EVP_CIPHER_CTX_cleanup(m_cipherCtx);

    return true;
}

bool OpenSslCryptoCodec::setTag(const Tag& tag) noexcept {
    if (!m_initDone) {
        ACSDK_ERROR(LX("setTagFailed").d("reason", "cipherIsNotInitialized"));
        return false;
    }
    if (!isAEADCipher()) {
        ACSDK_ERROR(LX("setTagFailed").d("reason", "notAEAD"));
        return false;
    }
    if (CodecType::Decoder != m_codecType) {
        ACSDK_ERROR(LX("setTagFailed").d("reason", "notDecoder"));
        return false;
    }

    OpenSslErrorCleanup errorCleanup{TAG};

    unsigned char* const tagData = const_cast<unsigned char*>(tag.data());

    if (OPENSSL_OK != EVP_CIPHER_CTX_ctrl(m_cipherCtx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tagData)) {
        ACSDK_ERROR(LX("setTagFailed").d("reason", "opensslError"));
        return false;
    }

    return true;
}

bool OpenSslCryptoCodec::isAEADCipher() noexcept {
    switch (m_algorithmType) {
        case AlgorithmType::AES_256_GCM:
        case AlgorithmType::AES_128_GCM:
            return true;
        default:
            return false;
    }
}

}  // namespace crypto
}  // namespace alexaClientSDK
