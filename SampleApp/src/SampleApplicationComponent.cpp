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

#include <acsdkCrypto/CryptoFactory.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkPkcs11/KeyStoreFactory.h>
#include <ContextManager/ContextManager.h>

#ifdef ACSDK_ACS_UTILS
#include <acsdkACSSampleApplicationOptions/ACSSampleApplicationOptionsComponent.h>
#else
#include <acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h>
#include <acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h>
#endif

#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <RegistrationManager/CustomerDataManagerFactory.h>

#include "SampleApp/LocaleAssetsManager.h"
#include "SampleApp/SampleApplicationComponent.h"
#include "SampleApp/UIManager.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

using namespace acsdkManufactory;
using namespace avsCommon::avs::initialization;

/**
 * @c UIManagerInterface factory that just forwards the instance of UIManager.
 *
 * @param uiManager The @c UIManager instance that will provide the implementation.
 * @return The implementation of @c CBLAuthRequesterInterface to use.
 */
static std::shared_ptr<acsdkSampleApplicationInterfaces::UIManagerInterface> createUIManagerInterface(
    const std::shared_ptr<sampleApp::UIManager>& uiManager) {
    return uiManager;
}

/**
 * Function that returns a factory to instantiate @c LocaleAssetsManagerInterface.
 *
 * @param requiresShutdownList - The vector of @c RequiresShutdown pointers to which the @c LocaleAssetsManager will be
 * added.
 * @return An std::function to instantiate @c LocaleAssetsManagerInterface.
 */
static std::function<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>&)>
getCreateLocaleAssetsManagerInterface(
    std::vector<std::shared_ptr<avsCommon::utils::RequiresShutdown>>& requiresShutdownList) {
    return
        [&requiresShutdownList](const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configNode) {
            auto manager = sampleApp::LocaleAssetsManager::createLocaleAssetsManager(configNode);
            if (manager) {
                requiresShutdownList.push_back(manager);
            }
            return manager;
        };
}

SampleApplicationComponent getComponent(
    std::unique_ptr<avsCommon::avs::initialization::InitializationParameters> initParams,
    std::vector<std::shared_ptr<avsCommon::utils::RequiresShutdown>>& requiresShutdownList,
    const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger) {
    return ComponentAccumulator<>()
        /// This initializes the SDK with the @c InitializationParameters. The factory method is primary,
        /// meaning it will be called before other factory methods in the manufactory.
        .addPrimaryFactory(AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams)))

    /// The below components specify commonly-changed application options, such as AuthDelegateInterface.
    /// Applications may want to replace these components with their own to specify custom implementations.
    /// Applications may also directly pass pre-built custom implementations of AuthDelegateInterface, Logger, or
    /// MetricRecorderInterface to the SampleApplicationOptionsComponent.
#ifdef ACSDK_ACS_UTILS
        .addComponent(acsdkSampleApplication::getSampleApplicationOptionsComponent())
#else
        .addComponent(
            acsdkSampleApplication::getSampleApplicationOptionsComponent(authDelegate, metricRecorder, logger))
        /// These interfaces are implemented for the Sample App, but applications may want to customize these (e.g. the
        /// CBLAuthRequesterInterface).
        .addRetainedFactory(
            acsdkSampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester::createCBLAuthRequesterInterface)
#endif
        .addRetainedFactory(getCreateLocaleAssetsManagerInterface(requiresShutdownList))
        .addRetainedFactory(sampleApp::UIManager::create)
        .addRetainedFactory(createUIManagerInterface)

        /// These objects are shared by many components in the SDK. Applications are not expected to change these.
        .addRetainedFactory(avsCommon::utils::DeviceInfo::createFromConfiguration)
        .addRetainedFactory(avsCommon::utils::configuration::ConfigurationNode::createRoot)
        .addUniqueFactory(avsCommon::utils::libcurlUtils::HttpPost::createHttpPostInterface)
        .addRetainedFactory(avsCommon::utils::timing::MultiTimer::createMultiTimer)
        .addRetainedFactory(contextManager::ContextManager::createContextManagerInterface)
        .addRetainedFactory(registrationManager::CustomerDataManagerFactory::createCustomerDataManagerInterface)
#ifdef ENABLE_PKCS11
        .addRetainedFactory(acsdkPkcs11::createKeyStore)
#else
        .addInstance(std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>())
#endif
        .addRetainedFactory(acsdkCrypto::createCryptoFactory);
}

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK
