/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/AVS/CapabilityConfiguration.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <AVSCommon/Utils/functional/hash.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

CapabilityConfiguration::CapabilityConfiguration(
    const std::unordered_map<std::string, std::string>& capabilityConfigurationIn) :
        capabilityConfiguration{capabilityConfigurationIn} {
}

bool operator==(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs) {
    if (lhs.capabilityConfiguration.size() != rhs.capabilityConfiguration.size()) {
        return false;
    }

    for (const auto& lhsIterator : lhs.capabilityConfiguration) {
        std::string lhsKey = lhsIterator.first;
        std::string lhsValue = lhsIterator.second;

        auto rhsIterator = rhs.capabilityConfiguration.find(lhsKey);
        if (rhsIterator == rhs.capabilityConfiguration.end()) {
            return false;
        }

        std::string rhsValue = rhsIterator->second;
        if (lhsValue.compare(rhsValue) != 0) {
            return false;
        }
    }

    return true;
}

bool operator!=(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs) {
    return !(lhs == rhs);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

size_t hash<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>::operator()(
    const alexaClientSDK::avsCommon::avs::CapabilityConfiguration& in) const {
    std::size_t seed = 0;
    /// Prefix and separator to construct a single string that represents a key/value pair.
    const std::string keyPrefix = "key:";
    const std::string valuePrefix = "value:";
    const std::string keyValueSeparator = ",";

    /// Sort the entries so that the hash values are the same. Since the map is unordered, we can't guarantee any order.
    std::vector<std::pair<std::string, std::string>> capabilityConfigurationElems(
        in.capabilityConfiguration.begin(), in.capabilityConfiguration.end());
    std::sort(capabilityConfigurationElems.begin(), capabilityConfigurationElems.end());

    for (const auto& configIterator : capabilityConfigurationElems) {
        const std::string mapEntryForHash =
            keyPrefix + configIterator.first + keyValueSeparator + valuePrefix + configIterator.second;
        alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, mapEntryForHash);
    }

    return seed;
};

}  // namespace std
