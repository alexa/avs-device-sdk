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

#include <acsdkCore/CoreComponent.h>
#include <acsdk/Crypto/CryptoFactory.h>
#ifdef ENABLE_PKCS11
#include <acsdk/Pkcs11/KeyStoreFactory.h>
#endif
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkShared/SharedComponent.h>
#include <ContextManager/ContextManager.h>
#include <acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h>

#ifdef ACSDK_ACS_UTILS
#include <acsdkACSSampleApplicationOptions/ACSSampleApplicationOptionsComponent.h>
#else
#include <acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>

#endif

#include <RegistrationManager/CustomerDataManagerFactory.h>

#include "IPCServerSampleApp/LocaleAssetsManager.h"
#include "IPCServerSampleApp/SampleApplicationComponent.h"

#ifdef METRICS_EXTENSION
#include <MetricsExtension/MetricsExtension.h>
#else
#include <acsdkDefaultSampleApplicationOptions/NullMetricRecorder.h>
#endif

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace alexaClientSDK;
using namespace acsdkManufactory;
using namespace avsCommon::avs::initialization;

SampleApplicationOptionsComponent getSampleApplicationOptionsComponent() {
    return ComponentAccumulator<>()
        .addComponent(acsdkShared::getComponent())
        .addPrimaryFactory(avsCommon::utils::logger::getConsoleLogger)
#ifdef METRICS_EXTENSION
        .addRetainedFactory(metrics::MetricsExtension::createMetricRecorderInterface)
#else
        .addRetainedFactory(acsdkDefaultSampleApplicationOptions::NullMetricRecorder::createMetricRecorderInterface)
#endif
        ;
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
            auto manager = LocaleAssetsManager::createLocaleAssetsManager(configNode);
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
#ifdef ACSDK_ACS_UTILS
        .addComponent(acsdkSampleApplication::getSampleApplicationOptionsComponent())
#else
        .addComponent(getSampleApplicationOptionsComponent())
#endif
        .addPrimaryFactory(AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams)))
        .addRetainedFactory(avsCommon::utils::configuration::ConfigurationNode::createRoot)
        .addUniqueFactory(avsCommon::utils::libcurlUtils::HttpPost::createHttpPostInterface)
        .addRetainedFactory(avsCommon::utils::timing::MultiTimer::createMultiTimer)
        .addRetainedFactory(getCreateLocaleAssetsManagerInterface(requiresShutdownList))
        .addRetainedFactory(contextManager::ContextManager::createContextManagerInterface)
        .addRetainedFactory(avsCommon::utils::DeviceInfo::createFromConfiguration)
        .addRetainedFactory(registrationManager::CustomerDataManagerFactory::createCustomerDataManagerInterface)
#ifdef ENABLE_PKCS11
        .addRetainedFactory(pkcs11::createKeyStore)
#else
        .addInstance(std::shared_ptr<cryptoInterfaces::KeyStoreInterface>())
#endif
        .addRetainedFactory(crypto::createCryptoFactory);
    ;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
