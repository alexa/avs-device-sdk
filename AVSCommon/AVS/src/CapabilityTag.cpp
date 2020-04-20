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

#include <tuple>

#include "AVSCommon/AVS/CapabilityTag.h"
#include "AVSCommon/Utils/functional/hash.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

CapabilityTag::CapabilityTag(
    const std::string& nameSpaceIn,
    const std::string& nameIn,
    const std::string& endpointIdIn,
    const utils::Optional<std::string>& instanceIdIn) :
        nameSpace{nameSpaceIn},
        name{nameIn},
        endpointId{endpointIdIn},
        instance{instanceIdIn} {
}

bool CapabilityTag::operator<(const CapabilityTag& rhs) const {
    return std::tie(nameSpace, name, endpointId, instance) <
           std::tie(rhs.nameSpace, rhs.name, rhs.endpointId, rhs.instance);
}

bool CapabilityTag::operator==(const CapabilityTag& rhs) const {
    return std::tie(nameSpace, name, endpointId, instance) ==
           std::tie(rhs.nameSpace, rhs.name, rhs.endpointId, rhs.instance);
}

bool CapabilityTag::operator!=(const CapabilityTag& rhs) const {
    return !(*this == rhs);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

size_t hash<alexaClientSDK::avsCommon::avs::CapabilityTag>::operator()(
    const alexaClientSDK::avsCommon::avs::CapabilityTag& in) const {
    std::size_t seed = 0;
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.nameSpace);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.name);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.endpointId);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.instance.valueOr(""));
    return seed;
};

}  // namespace std
