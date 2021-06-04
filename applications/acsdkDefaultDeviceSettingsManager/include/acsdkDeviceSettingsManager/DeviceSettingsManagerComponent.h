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

#ifndef ACSDKDEVICESETTINGSMANAGER_DEVICESETTINGSMANAGERCOMPONENT_H_
#define ACSDKDEVICESETTINGSMANAGER_DEVICESETTINGSMANAGERCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <acsdkDoNotDisturb/DoNotDisturbCapabilityAgent.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingsManagerBuilderBase.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkDeviceSettingsManager {

/**
 * Definition of a Manufactory Component for the default @c DeviceSettingsManagerBuilder.
 */
using DeviceSettingsManagerComponent = acsdkManufactory::Component<
    std::shared_ptr<settings::DeviceSettingsManager>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<settings::storage::DeviceSettingStorageInterface>>>;

/**
 * Creates an manufactory component that exports @c DeviceSettingsManager.
 *
 * @return A component.
 */
DeviceSettingsManagerComponent getComponent();

}  // namespace acsdkDeviceSettingsManager
}  // namespace alexaClientSDK

#endif  // ACSDKDEVICESETTINGSMANAGER_DEVICESETTINGSMANAGERCOMPONENT_H_
