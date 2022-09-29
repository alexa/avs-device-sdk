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

#ifndef ACSDK_CRYPTO_PRIVATE_OPENSSLERRORCLEANUP_H_
#define ACSDK_CRYPTO_PRIVATE_OPENSSLERRORCLEANUP_H_

#include <string>

#include <AVSCommon/Utils/Error/FinallyGuard.h>

namespace alexaClientSDK {
namespace crypto {

/**
 * @brief Helper class for handling OpenSSL errors.
 *
 * This class automatically clears OpenSSL error queue and logs errors to log on destruction. This helps with
 * troubleshooting OpenSSL errors.
 *
 * @ingroup Lib_acsdkCrypto
 */
class OpenSslErrorCleanup : public avsCommon::utils::error::FinallyGuard {
public:
    /**
     * @brief Constructs cleanup object.
     *
     * Initializes finally object to clean up OpenSSL error queue on destructor. Internally this method configures
     * @c FinallyGuard to call #clearAndLogOpenSslErrors() when destructor is called.
     *
     * @param[in] logTag Logging tag to use.
     */
    OpenSslErrorCleanup(const std::string& logTag) noexcept;

    /**
     * @brief Method clears openssl error queue and prints it to logger.
     *
     * This method clears OpenSSL error queue and prints errors to logger with a given tag.
     *
     * @param[in] logTag Logging tag to use.
     */
    static void clearAndLogOpenSslErrors(const std::string& logTag) noexcept;
};

}  // namespace crypto
}  // namespace alexaClientSDK

#endif  // ACSDK_CRYPTO_PRIVATE_OPENSSLERRORCLEANUP_H_
