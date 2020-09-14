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
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>

#include "acsdkShared/SharedComponent.h"

namespace alexaClientSDK {
namespace acsdkShared {

using namespace acsdkManufactory;
using namespace avsCommon;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsCommon::utils::timing;

Component<
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface>,
    std::shared_ptr<avsCommon::utils::timing::MultiTimer>>
getComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(ConfigurationNode::createRoot)
        .addUniqueFactory(HttpPost::createHttpPostInterface)
        .addRetainedFactory(ConstructorAdapter<MultiTimer>::get());
}

}  // namespace acsdkShared
}  // namespace alexaClientSDK
