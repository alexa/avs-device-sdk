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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_ADSLCOMPONENT_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_ADSLCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace adsl {

/**
 * Definition of a Manufactory component for Alexa directive sequencing.
 */
using ADSLComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>>;

/**
 * Creates an manufactory component that exports a shared pointer to @c DirectiveSequenceInterface.
 *
 * @return A component.
 */
ADSLComponent getComponent();

}  // namespace adsl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_ADSLCOMPONENT_H_
