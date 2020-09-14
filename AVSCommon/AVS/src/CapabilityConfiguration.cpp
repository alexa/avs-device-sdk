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

#include "AVSCommon/AVS/CapabilityConfiguration.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <AVSCommon/Utils/functional/hash.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

CapabilityConfiguration::Properties::Properties(
    bool isRetrievableIn,
    bool isProactivelyReportedIn,
    const std::vector<std::string>& supportedListIn,
    const alexaClientSDK::avsCommon::utils::Optional<bool>& isNonControllableIn) :
        isRetrievable{isRetrievableIn},
        isProactivelyReported{isProactivelyReportedIn},
        supportedList{supportedListIn},
        isNonControllable{isNonControllableIn} {
}

CapabilityConfiguration::CapabilityConfiguration(
    const std::unordered_map<std::string, std::string>& capabilityConfigurationIn) {
    auto it = capabilityConfigurationIn.find(CAPABILITY_INTERFACE_TYPE_KEY);
    if (it != capabilityConfigurationIn.end()) {
        type = it->second;
    }

    it = capabilityConfigurationIn.find(CAPABILITY_INTERFACE_VERSION_KEY);
    if (it != capabilityConfigurationIn.end()) {
        version = it->second;
    }

    it = capabilityConfigurationIn.find(CAPABILITY_INTERFACE_NAME_KEY);
    if (it != capabilityConfigurationIn.end()) {
        interfaceName = it->second;
    }

    it = capabilityConfigurationIn.find(CAPABILITY_INTERFACE_CONFIGURATIONS_KEY);
    if (it != capabilityConfigurationIn.end()) {
        additionalConfigurations.insert({it->first, it->second});
    }
}

CapabilityConfiguration::CapabilityConfiguration(
    const std::string& typeIn,
    const std::string& interfaceNameIn,
    const std::string& versionIn,
    const utils::Optional<std::string>& instanceNameIn,
    const utils::Optional<Properties>& propertiesIn,
    const CapabilityConfiguration::AdditionalConfigurations& additionalConfigurationsIn) :
        type{typeIn},
        interfaceName{interfaceNameIn},
        version{versionIn},
        instanceName{instanceNameIn},
        properties{propertiesIn},
        additionalConfigurations{additionalConfigurationsIn} {
}

bool operator==(const CapabilityConfiguration::Properties& lhs, const CapabilityConfiguration::Properties& rhs) {
    if ((lhs.isRetrievable != rhs.isRetrievable) || (lhs.isProactivelyReported != rhs.isProactivelyReported) ||
        (lhs.isNonControllable != rhs.isNonControllable)) {
        return false;
    }

    if (lhs.supportedList.size() != rhs.supportedList.size()) {
        return false;
    }

    for (size_t i = 0; i < lhs.supportedList.size(); ++i) {
        if (lhs.supportedList[i] != rhs.supportedList[i]) {
            return false;
        }
    }

    return true;
}

bool operator!=(const CapabilityConfiguration::Properties& lhs, const CapabilityConfiguration::Properties& rhs) {
    return !(lhs == rhs);
}

bool operator==(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs) {
    if ((lhs.interfaceName != rhs.interfaceName) || (lhs.version != rhs.version) || (lhs.type != rhs.type) ||
        (lhs.instanceName != rhs.instanceName) || (lhs.properties != rhs.properties)) {
        return false;
    }

    if (lhs.additionalConfigurations.size() != rhs.additionalConfigurations.size()) {
        return false;
    }

    for (const auto& lhsIterator : lhs.additionalConfigurations) {
        std::string lhsKey = lhsIterator.first;
        std::string lhsValue = lhsIterator.second;

        auto rhsIterator = rhs.additionalConfigurations.find(lhsKey);
        if (rhsIterator == rhs.additionalConfigurations.end()) {
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

bool operator==(
    const std::shared_ptr<CapabilityConfiguration>& lhs,
    const std::shared_ptr<CapabilityConfiguration>& rhs) {
    if (!lhs && !rhs) {
        return true;
    }
    if ((!lhs && rhs) || (lhs && !rhs)) {
        return false;
    }
    return (*lhs == *rhs);
}

bool operator!=(
    const std::shared_ptr<CapabilityConfiguration>& lhs,
    const std::shared_ptr<CapabilityConfiguration>& rhs) {
    return !(lhs == rhs);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

size_t hash<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>::operator()(
    const alexaClientSDK::avsCommon::avs::CapabilityConfiguration& in) const {
    std::size_t seed = 0;
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.interfaceName);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.type);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.instanceName.valueOr(""));
    return seed;
};

size_t hash<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>>::operator()(
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>& in) const {
    std::size_t seed = 0;
    if (in) {
        alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in->interfaceName);
        alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in->type);
        alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in->instanceName.valueOr(""));
    }
    return seed;
};

}  // namespace std
