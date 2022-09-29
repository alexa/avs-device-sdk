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

#include "acsdk/PresentationOrchestratorClient/PresentationOrchestratorClientFactory.h"

#include "acsdk/PresentationOrchestratorClient/private/PresentationOrchestratorClient.h"

namespace alexaClientSDK {
namespace presentationOrchestratorClient {
avsCommon::utils::Optional<PresentationOrchestratorClientFactory::PresentationOrchestratorClientExports>
PresentationOrchestratorClientFactory::create(
    const std::shared_ptr<presentationOrchestratorInterfaces::PresentationOrchestratorStateTrackerInterface>&
        stateTracker,
    const std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface>& visualTimeoutManager,
    const std::string& clientId) {
    auto poClient = PresentationOrchestratorClient::create(clientId, stateTracker, visualTimeoutManager);
    if (!poClient) {
        return avsCommon::utils::Optional<
            PresentationOrchestratorClientFactory::PresentationOrchestratorClientExports>();
    }

    PresentationOrchestratorClientFactory::PresentationOrchestratorClientExports exports = {
        poClient, poClient, poClient};

    return exports;
}
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK