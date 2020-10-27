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

#include <memory>

#include <acsdkEqualizerInterfaces/EqualizerConfigurationInterface.h>
#include <acsdkEqualizerInterfaces/EqualizerStorageInterface.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkManufactory/Import.h>

#include "acsdkEqualizerImplementations/MiscDBEqualizerStorage.h"
#include "acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h"

namespace alexaClientSDK {
namespace acsdkEqualizer {

using namespace acsdkManufactory;
using namespace avsCommon::sdkInterfaces;

Component<
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerStorageInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerConfigurationInterface>,
    Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>,
    Import<std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>>>
getComponent() {
    return ComponentAccumulator<>()
        .addRetainedFactory(acsdkEqualizer::MiscDBEqualizerStorage::createEqualizerStorageInterface)
        .addRetainedFactory(acsdkEqualizer::SDKConfigEqualizerConfiguration::createEqualizerConfigurationInterface);
}

}  // namespace acsdkEqualizer
}  // namespace alexaClientSDK
