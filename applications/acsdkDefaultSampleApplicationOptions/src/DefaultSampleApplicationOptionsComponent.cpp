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
#include <acsdkMetricRecorder/MetricRecorderComponent.h>
#include <acsdkShared/SharedComponent.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>

#include "acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

using namespace acsdkManufactory;
using namespace authorization::cblAuthDelegate;

/**
 * Returns a component for @c MetricRecorderInterface, using a pre-built implementation if available but otherwise
 * using the Sample App's default.
 *
 * @param metricRecorder Optional @c MetricRecorderInterface implementation to inject to the manufactory.
 * @return A component that exports the @c MetricRecorderInterface.
 */
static acsdkManufactory::Component<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>
getMetricRecorderComponent(const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) {
    /// If a custom metricRecorder is provided, add that implementation. Otherwise, use defaults.
    if (metricRecorder) {
        return ComponentAccumulator<>().addInstance(metricRecorder);
    }

    return ComponentAccumulator<>().addComponent(acsdkMetricRecorder::getComponent());
}

/**
 * Returns a component for @c AuthDelegateInterface, using a pre-built implementation if available but otherwise
 * using the Sample App's default.
 *
 * @param authDelegate Optional @c AuthDelegateInterface implementation to inject to the manufactory.
 * @return A component that exports the @c AuthDelegateInterface.
 */
static acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    acsdkManufactory::Import<std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::DeviceInfo>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>,
    acsdkManufactory::Import<std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>>
getAuthDelegateComponent(const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate) {
    /// If a custom authDelegate is provided, add that implementation. Otherwise, use defaults.
    if (authDelegate) {
        return ComponentAccumulator<>().addInstance(authDelegate);
    }

    return ComponentAccumulator<>()
        .addRetainedFactory(CBLAuthDelegate::createAuthDelegateInterface)
        .addRetainedFactory(SQLiteCBLAuthDelegateStorage::createCBLAuthDelegateStorageInterface);
}

/**
 * Returns a component for @c Logger, using a pre-built implementation if available but otherwise
 * using the Sample App's default.
 *
 * @param logger Optional @c Logger implementation to inject to the manufactory.
 * @return A component that exports the @c Logger.
 */
static acsdkManufactory::Component<std::shared_ptr<avsCommon::utils::logger::Logger>> getLoggerComponent(
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger) {
    if (logger) {
        return ComponentAccumulator<>().addInstance(logger);
    }

    return ComponentAccumulator<>()
#ifdef ANDROID_LOGGER
        .addPrimaryFactory(applicationUtilities::androidUtilities::AndroidLogger::getAndroidLogger)
#else
        .addPrimaryFactory(avsCommon::utils::logger::getConsoleLogger)
#endif
        ;
}

SampleApplicationOptionsComponent getSampleApplicationOptionsComponent(
    const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger) {
    return ComponentAccumulator<>()
        .addComponent(getAuthDelegateComponent(authDelegate))
        .addComponent(getLoggerComponent(logger))
        .addComponent(getMetricRecorderComponent(metricRecorder))
        .addComponent(acsdkShared::getComponent());
}

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK
