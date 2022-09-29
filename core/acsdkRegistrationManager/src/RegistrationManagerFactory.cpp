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

#include <RegistrationManager/CustomerDataManager.h>
#include <RegistrationManager/RegistrationManager.h>
#include <RegistrationManager/RegistrationNotifier.h>

#include "RegistrationManager/RegistrationManagerFactory.h"

namespace alexaClientSDK {
namespace registrationManager {

std::shared_ptr<registrationManager::RegistrationManagerInterface> createRegistrationManager(
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier) {
    auto registrationManager = RegistrationManager::createRegistrationManagerInterface(
        customerDataManager, notifier, connectionManager, directiveSequencer, metricRecorder);
    return registrationManager;
}

std::shared_ptr<registrationManager::RegistrationNotifierInterface> createRegistrationNotifier() {
    return RegistrationNotifier::createRegistrationNotifierInterface();
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
