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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_

#include <string>
#include <unordered_map>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Key for interface type in the @c CapabilityConfiguration map
static const std::string CAPABILITY_INTERFACE_TYPE_KEY = "type";
/// Key for interface name in the @c CapabilityConfiguration map
static const std::string CAPABILITY_INTERFACE_NAME_KEY = "interface";
/// Key for interface version in the @c CapabilityConfiguration map
static const std::string CAPABILITY_INTERFACE_VERSION_KEY = "version";
/// Key for interface configurations in the @c CapabilityConfiguration map
static const std::string CAPABILITY_INTERFACE_CONFIGURATIONS_KEY = "configurations";

/**
 * Class to encapsulate the capability configuration implemented by a capability agent.
 * This is entered as a key/value pair in the map.
 * key: CAPABILITY_INTERFACE_TYPE_KEY, value: The interface type being implemented.
 * key: CAPABILITY_INTERFACE_NAME_KEY, value: The name of the interface being implemented.
 * key: CAPABILITY_INTERFACE_VERSION_KEY, value: The version of the interface being implemented.
 * key: CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, value: A json of the configuration values for the interface being
 * implemented.
 */
class CapabilityConfiguration {
public:
    /**
     * Deleted default constructor.
     */
    CapabilityConfiguration() = delete;

    /**
     * Constructor to initialize with specific values.
     *
     * @param capabilityConfigIn The @c CapabilityConfiguration value for this instance.
     */
    CapabilityConfiguration(const std::unordered_map<std::string, std::string>& capabilityConfigurationIn);

    /// The @c CapabilityConfiguration value of this instance.
    const std::unordered_map<std::string, std::string> capabilityConfiguration;
};

/**
 * Operator == for @c CapabilityConfiguration
 *
 * @param rhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs);

/**
 * Operator != for @c CapabilityConfiguration
 *
 * @param rhs The left hand side of the != operation.
 * @param rhs The right hand side of the != operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs);

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

/**
 * @ std::hash() specialization defined for @c CapabilityConfiguration
 */
template <>
struct hash<alexaClientSDK::avsCommon::avs::CapabilityConfiguration> {
    size_t operator()(const alexaClientSDK::avsCommon::avs::CapabilityConfiguration& in) const;
};

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_
