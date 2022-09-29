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

#ifndef ACSDK_PROPERTIES_PRIVATE_ASN1HELPER_H_
#define ACSDK_PROPERTIES_PRIVATE_ASN1HELPER_H_

#include <vector>
#include <openssl/asn1t.h>
#include <string>

#include <acsdk/CryptoInterfaces/DigestType.h>
#include <acsdk/CryptoInterfaces/AlgorithmType.h>

namespace alexaClientSDK {
namespace properties {

using cryptoInterfaces::AlgorithmType;
using cryptoInterfaces::DigestType;

/**
 * @brief Helper for ASN.1 operations.
 *
 * @ingroup Lib_acsdkProperties
 */
struct Asn1Helper {
    /// @brief Byte vector type.
    typedef std::vector<unsigned char> Bytes;

    /**
     * @brief Sets optional integer value.
     *
     * Sets optional integer value (with default) to ASN.1 container. If the value doesn't match the
     * default value, and the container pointer is nullptr, a new container is allocated. If the value
     * matches the default, and the container pointer is not nullptr, the memory is released.
     *
     * The underlying ASN.1 library implementation doesn't have a notion of "default value", so the
     * entry must be nullptr, as otherwise it will be added to the output, which is against DER
     * specification.
     *
     * @param[in] asn1Integer Reference to container pointer. The method may release or allocate memory
     * and change pointer depending if the value matches the default one.
     * @param[in] value  Value to set.
     * @param[in] defaultValue Default value to check against.
     *
     * @return True if operation is successful.
     */
    static bool setOptInt(ASN1_INTEGER*& asn1Integer, int64_t value, int64_t defaultValue) noexcept;

    /**
     * @brief Gets optional integer value.
     *
     * Gets optional integer value. If the container memory is not allocated, a default value is returned.
     *
     * @param[in] asn1Integer Reference to container pointer. If the pointer is nullptr, the default value is
     * returned.
     * @param[out] value Value destination.
     * @param[in] defaultValue Default value to use if container pointer is nullptr.
     *
     * @return True if operation is successful.
     */
    static bool getOptInt(ASN1_INTEGER*& asn1Integer, int64_t& value, int64_t defaultValue) noexcept;

    /**
     * @brief Sets UTF8 string container value.
     *
     * @param asn1String Reference to container pointer. If pointer is nullptr, a new memory is allocated.
     * @param value Value to set.
     * @return True if operation is successful.
     */
    static bool setStr(ASN1_UTF8STRING*& asn1String, const std::string& value) noexcept;

    /**
     * @brief Gets UTF8 string from container.
     *
     * @param[in] asn1String Reference to container pointer. If pointer is nullptr, the operation fails.
     * @param[out] value Value destination.
     *
     * @return True if operation is successful.
     */
    static bool getStr(ASN1_UTF8STRING*& asn1String, std::string& value) noexcept;

    /**
     * @brief Sets binary data container value.
     *
     * @param[in] asn1String Reference to container pointer. If pointer is nullptr, a new memory is allocated.
     * @param[out] value Value destination.
     *
     * @return True if operation is successful.
     */
    static bool setData(ASN1_OCTET_STRING*& asn1String, const Bytes& value) noexcept;

    /**
     * @brief Gets binary data from container.
     *
     * @param[in] asn1String Reference to container pointer. If pointer is nullptr, the operation fails.
     * @param[out] value Value destination.
     *
     * @return True if operation is successful.
     */
    static bool getData(ASN1_OCTET_STRING*& asn1String, Bytes& value) noexcept;

    /**
     * @brief Maps algorithm type into ASN.1 value.
     *
     * Maps Crypto API cipher algorithm type value into ASN.1 value. The method fails, if it doesn't
     * recognize algorithm type.
     *
     * @param[in] type Algorithm type.
     * @param[out] asn1Type ASN.1 value.
     *
     * @return True if operation is successful.
     */
    static bool convertAlgTypeToAsn1(AlgorithmType type, int64_t& asn1Type) noexcept;

    /**
     * @brief Maps ASN.1 value into algorithm type.
     *
     * Maps ASN.1 constant into Crypto API cipher algorithm type. The method fails, if it doesn't
     * recognize algorithm type.
     *
     * @param[in] asn1Type ASN.1 constant.
     * @param[out] type Destination for algorithm type.
     *
     * @return True if operation is successful.
     */
    static bool convertAlgTypeFromAsn1(int64_t asn1Type, AlgorithmType& type) noexcept;

    /**
     * @brief Maps digest type into ASN.1.
     *
     * Maps Crypto API digest algorithm type value into ASN.1 value. The method fails, if it doesn't
     * recognize algorithm type.
     * @param[in] type Algorithm type.
     * @param[out] asn1Type ASN.1 constant.
     *
     * @return True if operation is successful.
     */
    static bool convertDigTypeToAsn1(DigestType type, int64_t& asn1Type) noexcept;

    /**
     * @brief Maps ASN.1 into digest type.
     *
     * Maps ASN.1 constant into Crypto API digest algorithm type. The method fails, if it doesn't
     * recognize algorithm type.
     *
     * @param[in] asn1Type ASN.1 constant.
     * @param[out] type Destination for algorithm type.
     *
     * @return True if operation is successful.
     */
    static bool convertDigTypeFromAsn1(int64_t asn1Type, DigestType& type) noexcept;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_PRIVATE_ASN1HELPER_H_
