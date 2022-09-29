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

#ifndef ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_DEFAULTSAMPLEAPPLICATIONOPTIONSCOMPONENT_H_
#define ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_DEFAULTSAMPLEAPPLICATIONOPTIONSCOMPONENT_H_

#include <acsdkManufactory/Component.h>
#include <acsdkSampleApplicationInterfaces/UIManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkSampleApplication {

/**
 * Definition of a Manufactory Component with default SampleApplication options.
 */
using SampleApplicationOptionsComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::utils::logger::Logger>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>;

/**
 * Get the @c Manufactory @c Component for the default @c SampleApplication options.
 *
 * For applications that have not transitioned to using the manufactory to instantiate SDK components, they can
 * provide pre-built custom implementations of the @c AuthDelegateInterface and @c MetricRecorderInterface.
 *
 * @param metricRecorder Optional pre-built implementation of @c MetricRecorderInterface to add to the
 * manufactory. Default is nullptr.
 * @param logger Optional pre-built implementation of @c Logger to add to the manufactory. Default is nullptr.
 * @return The @c Manufactory @c Component for the default @c SampleApplication options
 */
SampleApplicationOptionsComponent getSampleApplicationOptionsComponent(
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder = nullptr,
    const std::shared_ptr<avsCommon::utils::logger::Logger>& logger = nullptr);

}  // namespace acsdkSampleApplication
}  // namespace alexaClientSDK

#endif  // ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_DEFAULTSAMPLEAPPLICATIONOPTIONSCOMPONENT_H_
