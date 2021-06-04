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

#ifndef REGISTRATIONMANAGER_REGISTRATIONMANAGERCOMPONENT_H_
#define REGISTRATIONMANAGER_REGISTRATIONMANAGERCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

/**
 * Creates a manufactory component that exports RegistrationManager related implementation.
 *
 * @return A component.
 */
acsdkManufactory::Component<
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>
getComponent();

/**
 * Creates a manufactory component that exports RegistrationManager related implementation.
 *
 * @deprecated This is for backwards compatibility only to allow the application to inject an @c
 * CustomerDataManagerInterface. Prefer getComponent instead.
 * @return A component.
 */
acsdkManufactory::Component<
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>
getBackwardsCompatibleComponent();

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_REGISTRATIONMANAGERCOMPONENT_H_
