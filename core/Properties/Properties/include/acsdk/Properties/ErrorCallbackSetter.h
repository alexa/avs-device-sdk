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

#ifndef ACSDK_PROPERTIES_ERRORCALLBACKSETTER_H_
#define ACSDK_PROPERTIES_ERRORCALLBACKSETTER_H_

#include <cstdint>
#include <memory>
#include <string>

#include <acsdk/Properties/ErrorCallbackInterface.h>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Default number of retries when using error callback interface.
 *
 * Number of retries limits number of error handling attempts when implementation encounters a recoverable error. If
 * retry callback requests more than the given number of retries, the operation is considered as failed.
 *
 * @sa setErrorCallback()
 */
static constexpr uint32_t DEFAULT_MAX_RETRIES = 16u;

/**
 * @brief Unlimited number of retries when using error callback interface.
 *
 * If this value is used when setting error callback, the implementation will never give up on retries unless callback
 * tell to do so.
 *
 * @sa setErrorCallback()
 */
static constexpr uint32_t UNLIMITED_RETRIES = UINT32_MAX;

/**
 * @brief Sets an error callback.
 *
 * This method can both set a new callback or clear existing one if \a callback is nullptr. Changing callback affects
 * error handling of Property API methods that are called after the callback is changed.
 *
 * @param[in]   callback    New callback reference or nullptr to remove callback.
 * @param[in]   maxRetries  Maximum number of retries to use with this callback. If implementation encounters more
 *                          errors, than number of \a maxRetries plus one, the operation fails. If @ref
 *                          UNLIMITED_RETRIES value is specified, the implementation executes unlimited number of
 *                          retries until operation succeeds or \a callback indicates that operation must stop.
 * @param[out]  previous    Optional pointer to store previous callback.
 *
 * @return Boolean indicating operation success. On failure, contents of *previous is undefined and false is returned.
 *
 * @ingroup Lib_acsdkProperties
 */
bool setErrorCallback(
    const std::weak_ptr<ErrorCallbackInterface>& callback,
    uint32_t maxRetries = DEFAULT_MAX_RETRIES,
    std::weak_ptr<ErrorCallbackInterface>* previous = nullptr) noexcept;

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_ERRORCALLBACKSETTER_H_
