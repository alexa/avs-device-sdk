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
#include <acsdkShutdownManager/ShutdownManager.h>
#include <acsdkShutdownManager/ShutdownNotifier.h>
#include <acsdkStartupManager/StartupManager.h>
#include <acsdkStartupManager/StartupNotifier.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>

#include "acsdkShared/SharedComponent.h"

namespace alexaClientSDK {
namespace acsdkShared {

using namespace acsdkManufactory;
using namespace acsdkShutdownManager;
using namespace acsdkShutdownManagerInterfaces;
using namespace acsdkStartupManager;
using namespace acsdkStartupManagerInterfaces;
using namespace avsCommon;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::utils::timing;

SharedComponent getComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(ConfigurationNode::createRoot)
        .addRetainedFactory(avsCommon::utils::timing::MultiTimer::createMultiTimer)
        .addUniqueFactory(HttpPost::createHttpPostInterface)
        .addRetainedFactory(ShutdownManager::createShutdownManagerInterface)
        .addRetainedFactory(ShutdownNotifier::createShutdownNotifierInterface)
        .addRetainedFactory(StartupManager::createStartupManagerInterface)
        .addRetainedFactory(StartupNotifier::createStartupNotifierInterface);
}

}  // namespace acsdkShared
}  // namespace alexaClientSDK
