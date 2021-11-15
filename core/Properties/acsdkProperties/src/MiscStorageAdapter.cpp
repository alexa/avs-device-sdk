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

#include <acsdkProperties/MiscStorageAdapter.h>
#include <acsdkProperties/private/MiscStoragePropertiesFactory.h>
#include <acsdkProperties/private/Logging.h>

namespace alexaClientSDK {
namespace acsdkProperties {

using namespace alexaClientSDK::avsCommon::sdkInterfaces::storage;

/// String to identify log entries originating from this file.
/// @private
static const std::string TAG{"MiscStorageAdapter"};

std::shared_ptr<PropertiesFactoryInterface> createPropertiesFactory(
    const std::shared_ptr<MiscStorageInterface>& innerStorage,
    const std::shared_ptr<MiscStorageUriMapperInterface>& nameMapper) noexcept {
    auto res = MiscStoragePropertiesFactory::create(innerStorage, nameMapper);
    if (!res) {
        ACSDK_ERROR(LX("createPropertiesFactory"));
    }
    return res;
}

}  // namespace acsdkProperties
}  // namespace alexaClientSDK
