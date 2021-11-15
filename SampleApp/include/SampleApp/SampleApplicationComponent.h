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
#include <acsdkCryptoInterfaces/CryptoFactoryInterface.h>
#include <acsdkCryptoInterfaces/KeyStoreInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>

#ifndef ACSDK_ACS_UTILS
#include <CBLAuthDelegate/CBLAuthDelegateStorageInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>
#endif

#include <RegistrationManager/CustomerDataManagerInterface.h>

#include "SampleApp/UIManager.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

/**
 * Definition of the Manufactory component for the Sample App.
 *
 */
using SampleApplicationComponent = acsdkManufactory::Component<
#ifndef ACSDK_ACS_UTILS
    std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>,
#endif
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<sampleApp::UIManager>,
    std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>,
    std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>;

/**
 * Get the manufactory @c Component for acsdkSampleApp.
 *
 * For applications that have not transitioned to using the manufactory to instantiate SDK components, they can
 * provide pre-built custom implementations of the @c AuthDelegateInterface and @c MetricRecorderInterface.
 *
 * @param initParams The @c InitializationParameters to use when initializing the SDK Sample App.
 * @param requiresShutdownList The @c RequiresShutdown vector to which newly instantiated objects can be added.
 * @param authDelegate Optional pre-built implementation of @c AuthDelegateInterface to add to the manufactory. Default
 * is nullptr.
 * @param MetricRecorderInterface Optional pre-built implementation of @c MetricRecorderInterface to add to the
 * manufactory. Default is nullptr.
 * @param logger Optional pre-built implementation of @c Logger to add to the manufactory. Default is nullptr.
 * @return The manufactory @c Component for acsdkSampleApp.
 */
SampleApplicationComponent getComponent(
    std::unique_ptr<avsCommon::avs::initialization::InitializationParameters> initParams,
    std::vector<std::shared_ptr<avsCommon::utils::RequiresShutdown>>& requiresShutdownList,
    const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate = nullptr,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder = nullptr,
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger = nullptr);

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_SAMPLEAPPLICATIONCOMPONENT_H_
