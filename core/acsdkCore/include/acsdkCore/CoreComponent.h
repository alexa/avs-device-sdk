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

#ifndef ACSDKCORE_CORECOMPONENT_H_
#define ACSDKCORE_CORECOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Component.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Timing/MultiTimer.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <RegistrationManager/CustomerDataManager.h>

namespace alexaClientSDK {
namespace acsdkCore {

/**
 * Get a manufactory @c Component exporting core AVS client functionality.
 *
 * @return The dependency injection @c Component for acsdkCore.
 */
acsdkManufactory::Component<
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::timing::MultiTimer>>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<registrationManager::CustomerDataManager>>
getComponent();

}  // namespace acsdkCore
}  // namespace alexaClientSDK

#endif  // ACSDKCORE_CORECOMPONENT_H_
