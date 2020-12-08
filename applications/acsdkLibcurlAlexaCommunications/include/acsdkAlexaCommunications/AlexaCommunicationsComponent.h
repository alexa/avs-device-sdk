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

#ifndef ACSDKALEXACOMMUNICATIONS_ALEXACOMMUNICATIONSCOMPONENT_H_
#define ACSDKALEXACOMMUNICATIONS_ALEXACOMMUNICATIONSCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/EventTracerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlSetCurlOptionsCallbackFactoryInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>

namespace alexaClientSDK {
namespace acsdkAlexaCommunications {

/**
 * Manufactory Component definition for the default libcurl based implementation of AlexaCommunications.
 */
using AlexaCommunicationsComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::EventTracerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AVSConnectionManagerInterface,
        avsCommon::utils::libcurlUtils::LibcurlSetCurlOptionsCallbackFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>>;

/**
 * Get the @c Manufactory component for creating an instance of AVSConnectionMangerInterface.
 *
 * @return The @c Manufactory component for creating an instance of AVSConnectionMangerInterface.
 */
AlexaCommunicationsComponent getComponent();

}  // namespace acsdkAlexaCommunications
}  // namespace alexaClientSDK

#endif  // ACSDKALEXACOMMUNICATIONS_ALEXACOMMUNICATIONSCOMPONENT_H_
