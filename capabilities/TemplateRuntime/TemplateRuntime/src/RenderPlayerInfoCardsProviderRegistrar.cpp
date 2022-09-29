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
#include "acsdk/TemplateRuntime/private/RenderPlayerInfoCardsProviderRegistrar.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace templateRuntime {

/// String to identify log entries originating from this file.
#define TAG "RenderPlayerInfoCardsProviderRegistrar"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

bool RenderPlayerInfoCardsProviderRegistrar::registerProvider(
    const std::shared_ptr<RenderPlayerInfoCardsProviderInterface>& provider) {
    if (!provider) {
        ACSDK_ERROR(LX("registerProviderFailed").d("reason", "nullProvider"));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_providers.find(provider) != m_providers.end()) {
        ACSDK_ERROR(LX("registerProviderFailed").d("reason", "already registered"));
        return false;
    }

    m_providers.insert(provider);
    return true;
}

std::unordered_set<std::shared_ptr<RenderPlayerInfoCardsProviderInterface>> RenderPlayerInfoCardsProviderRegistrar::
    getProviders() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_providers;
}

}  // namespace templateRuntime
}  // namespace alexaClientSDK
