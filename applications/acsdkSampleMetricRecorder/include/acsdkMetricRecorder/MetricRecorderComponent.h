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

#ifndef ACSDKMETRICRECORDER_METRICRECORDERCOMPONENT_H_
#define ACSDKMETRICRECORDER_METRICRECORDERCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace acsdkMetricRecorder {

/**
 * Manufactory Component definition for null implementation of MetricRecorderInterface
 */
using MetricRecorderComponent =
    acsdkManufactory::Component<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>;

/**
 * Get the Manufactory component for creating instances of @c MetricRecorderInterface.
 *
 * @return The default @c Manufactory component for creating instances of @c MetricRecorderInterface.
 */
MetricRecorderComponent getComponent();

}  // namespace acsdkMetricRecorder
}  // namespace alexaClientSDK

#endif  // ACSDKMETRICRECORDER_METRICRECORDERCOMPONENT_H_
