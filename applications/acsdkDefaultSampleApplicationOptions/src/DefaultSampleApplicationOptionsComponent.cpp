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

#include "acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h"

namespace alexaClientSDK {
namespace acsdkSampleApplication {

using namespace acsdkManufactory;

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
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger) {
    return ComponentAccumulator<>()
        .addComponent(getLoggerComponent(logger))
        .addComponent(getMetricRecorderComponent(metricRecorder))
        .addComponent(acsdkShared::getComponent());
}

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK
