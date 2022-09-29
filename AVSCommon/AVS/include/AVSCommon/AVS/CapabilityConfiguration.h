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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/// Key for interface type in the @c CapabilityConfiguration map
static const auto CAPABILITY_INTERFACE_TYPE_KEY = "type";
/// Key for interface name in the @c CapabilityConfiguration map
static const auto CAPABILITY_INTERFACE_NAME_KEY = "interface";
/// Key for interface version in the @c CapabilityConfiguration map
static const auto CAPABILITY_INTERFACE_VERSION_KEY = "version";
/// Key for interface configurations in the @c CapabilityConfiguration map
static const auto CAPABILITY_INTERFACE_CONFIGURATIONS_KEY = "configurations";

/**
 * Class to encapsulate the capability configuration implemented by a capability agent.
 */
struct CapabilityConfiguration {
    /// Alexa interface type.
    static constexpr const char* ALEXA_INTERFACE_TYPE = "AlexaInterface";

    /// Alias for additional configurations.
    using AdditionalConfigurations = std::map<std::string, std::string>;

    /**
     * Structure representing the Capability Properties field.
     */
    struct Properties {
        /// boolean indicating if the capability properties can be retrieved using the @c ReportState Directive.
        bool isRetrievable;
        /// boolean indicating if the capability properties are proactively reported using the @c ChangeReport Event.
        bool isProactivelyReported;
        /// The list of supported properties of the capability agent.
        std::vector<std::string> supportedList;
        /// The optional nonControllable properties flag.
        avsCommon::utils::Optional<bool> isNonControllable;

        /**
         * Constructor.
         *
         * @param isRetrievableIn the isRetrievable properties flag.
         * @param isProactivelyReportedIn the isProactivelyReported properties flag.
         * @param supportedListIn the list of Supported property names.
         * @param isNonControllableIn the optional isNonControllable properties flag.
         */
        Properties(
            bool isRetrievableIn = false,
            bool isProactivelyReportedIn = false,
            const std::vector<std::string>& supportedListIn = std::vector<std::string>(),
            const avsCommon::utils::Optional<bool>& isNonControllableIn = avsCommon::utils::Optional<bool>());
    };

    /**
     * Default constructor enabled to be used only by Optional<>.
     */
    CapabilityConfiguration() = default;

    /**
     * Constructor to initialize with specific values.
     *
     * This is entered as a key/value pair in the given map.
     * key: CAPABILITY_INTERFACE_TYPE_KEY, value: The interface type being implemented.
     * key: CAPABILITY_INTERFACE_NAME_KEY, value: The name of the interface being implemented.
     * key: CAPABILITY_INTERFACE_VERSION_KEY, value: The version of the interface being implemented.
     * key: CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, value: A json of the configuration values for the interface being
     * implemented.
     *
     * @param capabilityConfigIn The @c CapabilityConfiguration value for this instance.
     * @deprecated This method will be removed soon.
     */
    CapabilityConfiguration(const std::unordered_map<std::string, std::string>& capabilityConfigurationIn);

    /**
     * Constructor.
     *
     * @param typeIn The Capability interface type string.
     * @param interfaceNameIn The Capability interface name string.
     * @param versionIn The Capability interface version string.
     * @param instanceNameIn The optional Capability interface instance string.
     * @param propertiesIn The optional @Properties structure.
     * @param additionalConfigurationsIn The optional @c AdditionalProperties map.
     */
    CapabilityConfiguration(
        const std::string& typeIn,
        const std::string& interfaceNameIn,
        const std::string& versionIn,
        const avsCommon::utils::Optional<std::string>& instanceNameIn = avsCommon::utils::Optional<std::string>(),
        const avsCommon::utils::Optional<Properties>& propertiesIn = avsCommon::utils::Optional<Properties>(),
        const AdditionalConfigurations& additionalConfigurationsIn = AdditionalConfigurations());

    /// The mandatory type of interface for this Capability.
    std::string type;
    /// The mandatory name of the Alexa interface.
    std::string interfaceName;
    /// The mandatory version of the Capability.
    std::string version;
    /// The optional instance name of the Capability.
    avsCommon::utils::Optional<std::string> instanceName;
    /// The optional properties field of the Capability.
    avsCommon::utils::Optional<Properties> properties;
    /// Any additional configuration fields of the Capability.
    /// Note: The values should be stringyfied JSON fields.
    AdditionalConfigurations additionalConfigurations;
};

/**
 * Operator == for @c CapabilityConfiguration::Properties
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const CapabilityConfiguration::Properties& lhs, const CapabilityConfiguration::Properties& rhs);

/**
 * Operator != for @c CapabilityConfiguration::Properties
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(const CapabilityConfiguration::Properties& lhs, const CapabilityConfiguration::Properties& rhs);

/**
 * Operator == for @c CapabilityConfiguration
 *
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs);

/**
 * Operator != for @c CapabilityConfiguration
 *
 * @param lhs The left hand side of the != operation.
 * @param rhs The right hand side of the != operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(const CapabilityConfiguration& lhs, const CapabilityConfiguration& rhs);

/**
 * Operator == for @c std::shared_ptr<CapabilityConfiguration>
 *
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(
    const std::shared_ptr<CapabilityConfiguration>& lhs,
    const std::shared_ptr<CapabilityConfiguration>& rhs);

/**
 * Operator != for @c std::shared_ptr<CapabilityConfiguration>
 *
 * @param lhs The left hand side of the != operation.
 * @param rhs The right hand side of the != operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(
    const std::shared_ptr<CapabilityConfiguration>& lhs,
    const std::shared_ptr<CapabilityConfiguration>& rhs);

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

/**
 * @ std::hash() specialization defined for @c CapabilityConfiguration shared pointer
 */
template <>
struct hash<std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>> {
    size_t operator()(const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityConfiguration>& in) const;
};

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYCONFIGURATION_H_
