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

#include <acsdk/Properties/ErrorCallbackSetter.h>
#include <acsdk/Properties/private/RetryExecutor.h>

namespace alexaClientSDK {
namespace properties {

bool setErrorCallback(
    const std::weak_ptr<ErrorCallbackInterface>& callback,
    uint32_t maxRetries,
    std::weak_ptr<ErrorCallbackInterface>* previous) noexcept {
    return RetryExecutor::setErrorCallback(callback, maxRetries, previous);
}

}  // namespace properties
}  // namespace alexaClientSDK
