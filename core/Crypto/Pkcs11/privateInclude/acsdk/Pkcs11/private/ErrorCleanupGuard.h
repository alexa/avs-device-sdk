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

#ifndef ACSDK_PKCS11_PRIVATE_ERRORCLEANUPGUARD_H_
#define ACSDK_PKCS11_PRIVATE_ERRORCLEANUPGUARD_H_

#include <AVSCommon/Utils/Error/FinallyGuard.h>

namespace alexaClientSDK {
namespace pkcs11 {

/**
 * @brief Error cleanup function on error.
 *
 * This object executes lambda on destruction only if success variable is false.
 *
 * @see alexaClientSDK::avsCommon::utils::error::FinallyGuard
 */
class ErrorCleanupGuard : public alexaClientSDK::avsCommon::utils::error::FinallyGuard {
public:
    /**
     * @brief Prepares lambda for execution.
     *
     * This method constructs @c FinallyGuard that will trigger @a successFlag check and optional @a cleanupFunction
     * execution on destruction.
     *
     * @param[in]   successFlag         Flag to check if @a cleanupFunction needs to be executed.
     * @param[in]   cleanupFunction     Function reference to execute if @a successFlag is false.
     */
    inline ErrorCleanupGuard(bool& successFlag, std::function<void()>&& cleanupFunction) noexcept :
            FinallyGuard{std::bind(executeCleanup, std::ref(successFlag), std::move(cleanupFunction))} {
    }

private:
    /**
     * @brief Executes lambda if flag is false.
     *
     * @param[in]   successFlag          Flag to check if @a cleanupFunction needs to be executed.
     * @param[in]   cleanupFunction      Function reference to execute if @a successFlag is false.
     */
    inline static void executeCleanup(bool& successFlag, const std::function<void()>& cleanupFunction) noexcept {
        if (!successFlag) {
            cleanupFunction();
        }
    }
};

}  // namespace pkcs11
}  // namespace alexaClientSDK

#endif  // ACSDK_PKCS11_PRIVATE_ERRORCLEANUPGUARD_H_
