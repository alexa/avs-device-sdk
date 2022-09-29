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

#include <climits>

#include <acsdk/Crypto/private/Logging.h>
#include <acsdk/Crypto/private/OpenSslDigest.h>
#include <acsdk/Crypto/private/OpenSslErrorCleanup.h>
#include <acsdk/Crypto/private/OpenSslTypeMapper.h>
#include <acsdk/Crypto/private/OpenSslTypes.h>

namespace alexaClientSDK {
namespace crypto {

using namespace alexaClientSDK::cryptoInterfaces;

/// String to identify log entries originating from this file.
/// @private
#define TAG "OpenSSL::Digest"

#if CHAR_BIT != 8
#error This source file must be modified to support 7-bit characters
#endif

std::unique_ptr<OpenSslDigest> OpenSslDigest::create(DigestType type) noexcept {
    std::unique_ptr<OpenSslDigest> res(new OpenSslDigest);

    if (!res->init(type)) {
        ACSDK_ERROR(LX("createFailed"));
        res.reset();
    }

    return res;
}

OpenSslDigest::OpenSslDigest() noexcept : m_ctx{nullptr} {
    OpenSslErrorCleanup errorCleanup{TAG};
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
    m_ctx = EVP_MD_CTX_new();
#else
    m_ctx = EVP_MD_CTX_create();
#endif
}

OpenSslDigest::~OpenSslDigest() noexcept {
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
    EVP_MD_CTX_free(m_ctx);
#else
    EVP_MD_CTX_destroy(m_ctx);
#endif
}

bool OpenSslDigest::init(DigestType type) noexcept {
    m_md = OpenSslTypeMapper::mapDigestToEvpMd(type);

    return OPENSSL_OK == EVP_DigestInit_ex(m_ctx, m_md, nullptr);
}

bool OpenSslDigest::process(const DataBlock& data_in) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    return data_in.empty() || OPENSSL_OK == EVP_DigestUpdate(m_ctx, &data_in[0], data_in.size());
}

bool OpenSslDigest::process(DataBlock::const_iterator begin, DataBlock::const_iterator end) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    return begin == end || OPENSSL_OK == EVP_DigestUpdate(m_ctx, &*begin, end - begin);
}

bool OpenSslDigest::processByte(unsigned char value) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    return OPENSSL_OK == EVP_DigestUpdate(m_ctx, &value, sizeof(value));
}

bool OpenSslDigest::processUInt8(uint8_t value) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    return OPENSSL_OK == EVP_DigestUpdate(m_ctx, &value, sizeof(value));
}

bool OpenSslDigest::processUInt16(uint16_t value) noexcept {
    const DataBlock bigEndianValue{(DataBlock::value_type)(value >> 8), (DataBlock::value_type)value};
    return process(bigEndianValue);
}

bool OpenSslDigest::processUInt32(uint32_t value) noexcept {
    const DataBlock bigEndianValue{(DataBlock::value_type)(value >> CHAR_BIT * 3),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 2),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 1),
                                   (DataBlock::value_type)value};
    return process(bigEndianValue);
}

bool OpenSslDigest::processUInt64(uint64_t value) noexcept {
    const DataBlock bigEndianValue{(DataBlock::value_type)(value >> CHAR_BIT * 7),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 6),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 5),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 4),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 3),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 2),
                                   (DataBlock::value_type)(value >> CHAR_BIT * 1),
                                   (DataBlock::value_type)value};
    return process(bigEndianValue);
}

bool OpenSslDigest::processString(const std::string& value) noexcept {
    return value.empty() || OPENSSL_OK == EVP_DigestUpdate(m_ctx, value.c_str(), value.size());
}

bool OpenSslDigest::finalize(DataBlock& dataOut) noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
    size_t index = dataOut.size();
    dataOut.resize(index + EVP_MD_size(m_md));
    bool res = (OPENSSL_OK == EVP_DigestFinal_ex(m_ctx, &dataOut[index], nullptr));
    reset();
    return res;
}

bool OpenSslDigest::reset() noexcept {
    OpenSslErrorCleanup errorCleanup{TAG};
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_NUMBER_1_1_0
    return OPENSSL_OK == EVP_MD_CTX_reset(m_ctx);
#else
    EVP_MD_CTX_cleanup(m_ctx);
    return OPENSSL_OK == EVP_DigestInit_ex(m_ctx, m_md, nullptr);
#endif
}

}  // namespace crypto
}  // namespace alexaClientSDK
