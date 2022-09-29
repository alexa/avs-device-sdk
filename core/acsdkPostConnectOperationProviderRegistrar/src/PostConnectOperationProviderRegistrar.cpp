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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkPostConnectOperationProviderRegistrar/PostConnectOperationProviderRegistrar.h"

namespace alexaClientSDK {
namespace acsdkPostConnectOperationProviderRegistrar {

/// String to identify log entries originating from this file.
#define TAG "PostConnectOperationProviderRegistrar"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace acsdkStartupManagerInterfaces;
using namespace acsdkPostConnectOperationProviderRegistrarInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

std::shared_ptr<PostConnectOperationProviderRegistrarInterface> PostConnectOperationProviderRegistrar::
    createPostConnectOperationProviderRegistrarInterface(
        const std::shared_ptr<StartupNotifierInterface>& startupNotifier) {
    if (!startupNotifier) {
        ACSDK_ERROR(LX("createPostConnectOperationProviderRegistrarFailed").d("reason", "nullStartupNotifier"));
        return nullptr;
    }

    std::shared_ptr<PostConnectOperationProviderRegistrar> registrar(new PostConnectOperationProviderRegistrar());
    startupNotifier->addObserver(registrar);
    return registrar;
}

bool PostConnectOperationProviderRegistrar::registerProvider(
    const std::shared_ptr<PostConnectOperationProviderInterface>& provider) {
    if (!provider) {
        ACSDK_ERROR(LX("registerProviderFailed").d("reason", "nullProvider"));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_onStartupHasBeenCalled) {
        ACSDK_ERROR(LX("registerProviderFailed").d("reason", "onStartupHasBeenCalled"));
        return false;
    }
    m_providers.push_back(provider);
    return true;
}

Optional<std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>> PostConnectOperationProviderRegistrar::
    getProviders() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_onStartupHasBeenCalled) {
        ACSDK_ERROR(LX("getProvidersFailed").d("reason", "!onStartupHasBeenCalled"));
        return Optional<std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>>();
    }

    return m_providers;
}

bool PostConnectOperationProviderRegistrar::startup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onStartupHasBeenCalled = true;
    return true;
}

PostConnectOperationProviderRegistrar::PostConnectOperationProviderRegistrar() : m_onStartupHasBeenCalled{false} {
}

}  // namespace acsdkPostConnectOperationProviderRegistrar
}  // namespace alexaClientSDK
