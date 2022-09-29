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

#include <acsdk/Pkcs11/private/PKCS11KeyDescriptor.h>

namespace alexaClientSDK {
namespace pkcs11 {

/// String to identify log entries originating from this file.
/// @private
#define TAG "pkcs11::KeyDescriptor"

using namespace alexaClientSDK::cryptoInterfaces;

/// @private
static constexpr CK_ULONG AES_256_KEY_SIZE = 32;
/// @private
static constexpr CK_ULONG AES_128_KEY_SIZE = 16;

bool PKCS11KeyDescriptor::mapAlgorithmToKeyParams(AlgorithmType algorithmType, CK_KEY_TYPE& keyType, CK_ULONG& keyLen) {
    switch (algorithmType) {
        case AlgorithmType::AES_256_CBC:
        case AlgorithmType::AES_256_CBC_PAD:
        case AlgorithmType::AES_256_GCM:
            keyType = CKK_AES;
            keyLen = AES_256_KEY_SIZE;
            return true;
        case AlgorithmType::AES_128_CBC:
        case AlgorithmType::AES_128_CBC_PAD:
        case AlgorithmType::AES_128_GCM:
            keyType = CKK_AES;
            keyLen = AES_128_KEY_SIZE;
            return true;
        default:
            keyType = UNDEFINED_KEY_TYPE;
            keyLen = 0;
            return false;
    }
}

PKCS11KeyDescriptor::PKCS11KeyDescriptor(const std::string& objectLabel, AlgorithmType algorithmType) noexcept :
        objectLabel{objectLabel} {
    mapAlgorithmToKeyParams(algorithmType, keyType, keyLen);
}

PKCS11KeyDescriptor::PKCS11KeyDescriptor(
    const std::string& objectLabel,
    CK_KEY_TYPE keyType,
    CK_ULONG keyLen) noexcept :
        objectLabel{objectLabel},
        keyType{keyType},
        keyLen{keyLen} {
}

}  // namespace pkcs11
}  // namespace alexaClientSDK

namespace std {

bool equal_to<alexaClientSDK::pkcs11::PKCS11KeyDescriptor>::operator()(
    const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg1,
    const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg2) const noexcept {
    return &arg1 == &arg2 ||
           (arg1.keyType == arg2.keyType && arg1.keyLen == arg2.keyLen && arg1.objectLabel == arg2.objectLabel);
}

std::size_t hash<alexaClientSDK::pkcs11::PKCS11KeyDescriptor>::operator()(
    const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg) const noexcept {
    return hash<std::string>()(arg.objectLabel) ^ hash<CK_ULONG>()(arg.keyType) ^ hash<CK_ULONG>()(arg.keyLen);
}

std::ostream& operator<<(std::ostream& out, const alexaClientSDK::pkcs11::PKCS11KeyDescriptor& arg) noexcept {
    out << arg.objectLabel << "/";
    switch (arg.keyType) {
        case CKK_AES:
            out << "AES";
            break;
        default:
            out << arg.keyType;
            break;
    }
    return out << "/" << arg.keyLen;
}

}  // namespace std
