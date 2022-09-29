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

#include "acsdk/SDKClient/SDKClientBuilder.h"

namespace alexaClientSDK {
namespace sdkClient {

/// String to identify log entries originating from this file.
#define TAG "SDKClientBuilder"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<SDKClientRegistry> SDKClientBuilder::build() {
    if (m_clients.empty()) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "No feature clients to build"));
        return nullptr;
    }

    // Resolve dependencies and calculate build order
    // One core feature should have zero dependencies, identify it first
    internal::TypeRegistry providedTypes;

    auto sdkClientRegistry = std::shared_ptr<SDKClientRegistry>(new SDKClientRegistry());

    auto modified = true;
    while (modified && !m_clients.empty()) {
        modified = false;
        for (auto it = m_clients.begin(); it != m_clients.end();) {
            const auto& client = *it;
            if (client->client->getRequiredTypes().typeDifferenceIsEmpty(providedTypes)) {
                ACSDK_DEBUG0(LX(__func__).d("constructing", client->client->name()));
                auto newClient = client->constructorFn(sdkClientRegistry);
                if (!newClient) {
                    ACSDK_CRITICAL(LX("buildFailed")
                                       .d("reason", "Failed to instantiate client")
                                       .d("client", client->client->name()));
                    return nullptr;
                }

                sdkClientRegistry->registerClient(client->typeId, std::move(newClient));

                it = m_clients.erase(it);
                modified = true;
            } else {
                ++it;
            }
        }

        // Update the list of provided types
        for (auto& componentPair : sdkClientRegistry->m_componentMapping) {
            providedTypes.addTypeIndex(componentPair.first);
        }
    }

    if (!m_clients.empty()) {
        for (const auto& client : m_clients) {
            const auto missingDeps = client->client->getRequiredTypes().typeDifference(providedTypes);

            ACSDK_ERROR(LX("buildFailed")
                            .d("reason", "Unsatisfied dependency building client")
                            .d("client", client->client->name())
                            .d("missing dependencies", missingDeps));
        }
        return nullptr;
    }

    // Cleanup, unique_ptrs are no longer valid
    m_clients.clear();

    // Finalize initialization of the combined client
    if (!sdkClientRegistry->initialize()) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "configuration failed"));
        return nullptr;
    }

    return sdkClientRegistry;
}

void SDKClientBuilder::withFeature(std::shared_ptr<Client> client) {
    if (!client || !client->client) {
        ACSDK_ERROR(LX("withFeatureFailed").d("reason", "Null client"));
        return;
    }

    if (std::find_if(m_clients.begin(), m_clients.end(), [client](const std::shared_ptr<Client>& other) {
            return client->typeId == other->typeId;
        }) != m_clients.end()) {
        ACSDK_WARN(LX("withFeatureFailed").d("reason", "Client already exists").d("type", client->client->name()));
    } else {
        m_clients.push_back(std::move(client));
    }
}

}  // namespace sdkClient
}  // namespace alexaClientSDK