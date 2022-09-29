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

#include "acsdk/PresentationOrchestratorStateTracker/PresentationOrchestratorStateTrackerFactory.h"

#include "acsdk/PresentationOrchestratorStateTracker/private/PresentationOrchestratorStateTracker.h"

namespace alexaClientSDK {
namespace presentationOrchestratorStateTracker {
avsCommon::utils::Optional<PresentationOrchestratorStateTrackerFactory::PresentationOrchestratorStateTrackerExports>
PresentationOrchestratorStateTrackerFactory::create(
    const std::shared_ptr<afml::ActivityTrackerInterface>& activityTrackerInterface) {
    auto poStateTracker = PresentationOrchestratorStateTracker::create(activityTrackerInterface);
    if (!poStateTracker) {
        return avsCommon::utils::Optional<PresentationOrchestratorStateTrackerExports>();
    }

    PresentationOrchestratorStateTrackerExports exports = {poStateTracker, poStateTracker};
    return exports;
}
}  // namespace presentationOrchestratorStateTracker
}  // namespace alexaClientSDK