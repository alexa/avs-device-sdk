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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATIONCOMPONENT_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATIONCOMPONENT_H_

#include <istream>
#include <memory>
#include <vector>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <CBLAuthDelegate/CBLAuthDelegateStorageInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#include <RegistrationManager/CustomerDataManager.h>

#include "SampleApp/UIManager.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

/**
 * Get the manufactory @c Component for acsdkSampleApp.
 *
 * @return The manufactory @c Component for acsdkSampleApp.
 */
acsdkManufactory::Component<
    std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>,
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<registrationManager::CustomerDataManager>,
    std::shared_ptr<sampleApp::UIManager>>
getComponent(const std::vector<std::shared_ptr<std::istream>>& jsonStreams);

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATIONCOMPONENT_H_
