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
#include <AVSCommon/Utils/functional/hash.h>

#include "AVSCommon/AVS/ComponentConfiguration.h"

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// String to identify log entries originating from this file.
static const std::string TAG("ComponentConfiguration");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Helper Function to determine if a configuration is valid
 *
 * @param name The name of the configuration
 * @param version The version of the configuration
 *
 * @return Bool indicated if configuration is valid. True if valid, false if invalid.
 */
static bool isValidConfiguration(std::string name, std::string version) {
    // Invalid if version is not a dot separated alpha numeric string
    for (auto it = version.begin(); it != version.end(); ++it) {
        const unsigned char& c = *it;

        if (!std::isalnum(c) && c != '.') {
            ACSDK_ERROR(LX(__func__).m("invalid component version").d("name", name).d("version", version));
            return false;
        }
        // Version must have characters between dots
        if (c == '.' && it + 1 != version.end() && *(it + 1) == '.') {
            ACSDK_ERROR(LX(__func__).m("invalid component version").d("name", name).d("version", version));
            return false;
        }
    }

    // Valid if configuration is not empty
    if (name.length() == 0 || version.length() == 0) {
        ACSDK_ERROR(LX(__func__).m("component can not be empty").d("name", name).d("version", version));
        return false;
    }

    return true;
}

ComponentConfiguration::ComponentConfiguration(std::string name, std::string version) : name{name}, version{version} {
}

std::shared_ptr<ComponentConfiguration> ComponentConfiguration::createComponentConfiguration(
    std::string name,
    std::string version) {
    if (isValidConfiguration(name, version)) {
        return std::make_shared<ComponentConfiguration>(ComponentConfiguration({name, version}));
    }

    return nullptr;
}

bool operator==(const ComponentConfiguration& lhs, const ComponentConfiguration& rhs) {
    return lhs.name == rhs.name;
}

bool operator!=(const ComponentConfiguration& lhs, const ComponentConfiguration& rhs) {
    return !(lhs == rhs);
}

bool operator==(const std::shared_ptr<ComponentConfiguration> lhs, const std::shared_ptr<ComponentConfiguration> rhs) {
    if (!lhs && !rhs) {
        return true;
    }
    if ((!lhs && rhs) || (lhs && !rhs)) {
        return false;
    }

    return *lhs == *rhs;
}

bool operator!=(const std::shared_ptr<ComponentConfiguration> lhs, const std::shared_ptr<ComponentConfiguration> rhs) {
    return !(lhs == rhs);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

size_t hash<alexaClientSDK::avsCommon::avs::ComponentConfiguration>::operator()(
    const alexaClientSDK::avsCommon::avs::ComponentConfiguration& in) const {
    std::size_t seed = 0;
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.name);
    return seed;
};

size_t hash<std::shared_ptr<alexaClientSDK::avsCommon::avs::ComponentConfiguration>>::operator()(
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::ComponentConfiguration> in) const {
    std::size_t seed = 0;
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in->name);
    return seed;
};

}  // namespace std
