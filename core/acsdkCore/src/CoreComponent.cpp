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
#include <acsdkManufactory/ConstructorAdapter.h>
#include <acsdkShared/SharedComponent.h>
#include <ContextManager/ContextManager.h>

#include "acsdkCore/CoreComponent.h"

namespace alexaClientSDK {
namespace acsdkCore {

using namespace acsdkManufactory;

Component<
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::timing::MultiTimer>>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<registrationManager::CustomerDataManager>>
getComponent() {
    return ComponentAccumulator<>()
        .addComponent(acsdkShared::getComponent())
        .addRetainedFactory(contextManager::ContextManager::createContextManagerInterface)
        .addRetainedFactory(avsCommon::utils::DeviceInfo::createFromConfiguration)
        .addRetainedFactory(ConstructorAdapter<registrationManager::CustomerDataManager>::get());
}

}  // namespace acsdkCore
}  // namespace alexaClientSDK
