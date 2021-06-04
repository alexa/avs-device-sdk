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
#include <Metrics/MetricRecorder.h>
#include <Metrics/SampleMetricSink.h>

#include "acsdkMetricRecorder/MetricRecorderComponent.h"

namespace alexaClientSDK {
namespace acsdkMetricRecorder {

using namespace acsdkManufactory;
using namespace metrics::implementations;

MetricRecorderComponent getComponent() {
    return ComponentAccumulator<>()
        .addUniqueFactory(SampleMetricSink::createMetricSinkInterface)
        .addRetainedFactory(MetricRecorder::createMetricRecorderInterface);
}

}  // namespace acsdkMetricRecorder
}  // namespace alexaClientSDK
