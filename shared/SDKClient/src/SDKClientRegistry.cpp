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

#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <acsdkShutdownManager/ShutdownManager.h>

#include "acsdk/SDKClient/SDKClientRegistry.h"

namespace alexaClientSDK {
namespace sdkClient {
/// String to identify log entries originating from this file.
#define TAG "SDKClientRegistry"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

SDKClientRegistry::SDKClientRegistry() : avsCommon::utils::RequiresShutdown(TAG) {
    m_shutdownNotifier = acsdkShutdownManager::ShutdownNotifier::createShutdownNotifierInterface();
    m_shutdownManager = acsdkShutdownManager::ShutdownManager::createShutdownManagerInterface(m_shutdownNotifier);
    if (!m_shutdownNotifier || !m_shutdownManager) {
        ACSDK_ERROR(LX("constructorFailed").d("reason", "null shutdownManager"));
    }
}

bool SDKClientRegistry::initialize() {
    for (const auto& client : m_clientMapping) {
        if (!client.second->configure(shared_from_this())) {
            ACSDK_ERROR(
                LX("initializeFailed").d("reason", "Client configuration failed").d("client", client.first.getName()));
            return false;
        }
    }
    return true;
}

bool SDKClientRegistry::registerClient(
    const avsCommon::utils::TypeIndex& clientTypeId,
    std::shared_ptr<FeatureClientInterface> client) {
    if (!client) {
        ACSDK_ERROR(LX("registerClientFailed").d("reason", "null client"));
        return false;
    }
    if (m_clientMapping.count(clientTypeId)) {
        ACSDK_ERROR(LX("registerClientFailed").d("reason", "Client already registered"));
        return false;
    }
    if (m_shutdownNotifier) {
        m_shutdownNotifier->addObserver(client);
    }
    m_clientMapping[clientTypeId] = std::move(client);
    return true;
}

bool SDKClientRegistry::registerComponent(
    const avsCommon::utils::TypeIndex& clientTypeId,
    std::shared_ptr<void> component) {
    if (!component) {
        ACSDK_WARN(LX("registerComponentFailed").d("reason", "Attempt to register null pointer"));
        return false;
    }

    if (m_componentMapping.count(clientTypeId)) {
        // Duplicate component
        ACSDK_WARN(
            LX("registerComponentFailed").d("reason", "Component already exists").d("type", clientTypeId.getName()));
        return false;
    }
    m_componentMapping[clientTypeId] = std::move(component);
    return true;
}

void SDKClientRegistry::doShutdown() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
    }
}

bool SDKClientRegistry::addFeature(
    const avsCommon::utils::TypeIndex& clientTypeId,
    std::unique_ptr<FeatureClientBuilderInterface> featureBuilder,
    std::function<std::shared_ptr<FeatureClientInterface>(const std::shared_ptr<SDKClientRegistry>&)> constructorFn) {
    if (!featureBuilder) {
        ACSDK_ERROR(LX("addFeatureFailed").d("reason", "null featureBuilder"));
        return false;
    }
    if (!constructorFn) {
        ACSDK_ERROR(LX("addFeatureFailed").d("reason", "empty constructor function"));
        return false;
    }
    // Verify that each required type is already available
    const auto& requiredTypes = featureBuilder->getRequiredTypes();
    for (auto it = requiredTypes.cbegin(); it != requiredTypes.cend(); ++it) {
        auto componentIt = m_componentMapping.find(*it);
        if (componentIt == m_componentMapping.end()) {
            ACSDK_ERROR(LX("addFeatureFailed").d("reason", "Missing required dependency").d("type", it->getName()));
            return false;
        }
    }
    auto client = constructorFn(shared_from_this());
    if (!client) {
        ACSDK_ERROR(LX("addFeatureFailed").d("reason", "Feature construction failed"));
        return false;
    }

    if (!client->configure(shared_from_this())) {
        ACSDK_ERROR(LX("addFeatureFailed").d("reason", "Feature configuration failed"));
        return false;
    }

    return registerClient(clientTypeId, std::move(client));
}
}  // namespace sdkClient
}  // namespace alexaClientSDK
