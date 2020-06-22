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
#include <acsdkCore/CoreComponent.h>
#include <acsdkShared/SharedComponent.h>
#include <ContextManager/ContextManager.h>

#ifdef ACSDK_ACS_UTILS
#include <acsdkACSSampleApplicationOptions/ACSSampleApplicationOptionsComponent.h>
#else
#include <acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h>
#endif

#include "SampleApp/LocaleAssetsManager.h"
#include "SampleApp/SampleApplicationComponent.h"
#include "SampleApp/UIManager.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

using namespace acsdkManufactory;
using namespace avsCommon::avs::initialization;

/**
 * @c CBLAuthRequesterInterface factory that just forwards the instance of UIManager.
 *
 * @param uiManager The @c UIManager instance that will provide the implementation.
 * @return The implementation of @c CBLAuthRequesterInterface to use.
 */
static std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface> createCBLAuthRequesterInterface(
    const std::shared_ptr<sampleApp::UIManager>& uiManager) {
    return uiManager;
}

Component<
    std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>,
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<registrationManager::CustomerDataManager>,
    std::shared_ptr<sampleApp::UIManager>>
getComponent(const std::vector<std::shared_ptr<std::istream>>& jsonStreams) {
    return ComponentAccumulator<>()
#ifdef ACSDK_ACS_UTILS
        .addComponent(acsdkACSSampleApplicationOptions::getComponent())
#else
        .addComponent(acsdkDefaultSampleApplicationOptions::getComponent())
#endif
        .addPrimaryFactory(AlexaClientSDKInit::getCreateAlexaClientSDKInit(jsonStreams))
        .addComponent(acsdkShared::getComponent())
        .addComponent(acsdkCore::getComponent())
        .addRetainedFactory(createCBLAuthRequesterInterface)
        .addRetainedFactory(sampleApp::LocaleAssetsManager::createLocaleAssetsManagerInterface)
        .addRetainedFactory(sampleApp::UIManager::create);
}

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK
