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

#include <acsdkManufactory/ComponentAccumulator.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <RegistrationManager/RegistrationManager.h>
#include <RegistrationManager/RegistrationNotifier.h>

#include "RegistrationManager/RegistrationManagerComponent.h"

namespace alexaClientSDK {
namespace registrationManager {

using namespace acsdkManufactory;

acsdkManufactory::Component<
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>
getComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(CustomerDataManager::createCustomerDataManagerInteface)
        .addRetainedFactory(RegistrationManager::createRegistrationManagerInterface)
        .addRetainedFactory(RegistrationNotifier::createRegistrationNotifierInterface);
}

acsdkManufactory::Component<
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>
getBackwardsCompatibleComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(RegistrationManager::createRegistrationManagerInterface)
        .addRetainedFactory(RegistrationNotifier::createRegistrationNotifierInterface);
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
