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
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPostInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>

namespace alexaClientSDK {
namespace acsdkDefaultSampleApplicationOptions {

/**
 * Get the @c Manufactory @c Component for the default @c SampleApplication options
 *
 * @return The @c Manufactory @c Component for the default @c SampleApplication options
 */
acsdkManufactory::Component<
    acsdkManufactory::Import<std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>>,
    acsdkManufactory::Import<std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface>>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::utils::logger::Logger>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>
getComponent();

}  // namespace acsdkDefaultSampleApplicationOptions
}  // namespace alexaClientSDK

#endif  // ACSDKDEFAULTSAMPLEAPPLICATIONOPTIONS_DEFAULTSAMPLEAPPLICATIONOPTIONSCOMPONENT_H_
