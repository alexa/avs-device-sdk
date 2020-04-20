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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYTAG_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYTAG_H_

#include <ostream>
#include <string>

#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Structure containing values for namespace, name, endpointId and optionally instance which are intended for
 * identifying AVS Messages (Directives, Events, State, and Exceptions).
 */
struct CapabilityTag {
    /**
     * Constructor.
     *
     * @param namespace The namespace value for this message.
     * @param name The name for this message.
     * @param endpointId The endpoint id used to identify the target / source endpoint.
     * @param instanceId Optional value for specifying an specific capability instance. This field should be left
     * empty if the capability does not support multiple instances.
     */
    CapabilityTag(
        const std::string& nameSpace,
        const std::string& name,
        const std::string& endpointId,
        const utils::Optional<std::string>& instanceId = utils::Optional<std::string>());

    /**
     * Copy constructor.
     *
     * @param other Object used to initialize the new object.
     */
    CapabilityTag(const CapabilityTag& other) = default;

    /// The namespace value of this message.
    const std::string nameSpace;

    /// The name value of this message.
    const std::string name;

    /// The endpoint id of this message.
    const std::string endpointId;

    /// The capability instance relative to this message if applicable; otherwise, an empty object.
    const utils::Optional<std::string> instance;

    /**
     *  @name Comparison operators.
     *
     *  Compare the current capability tag against a second object.
     *
     *  @param rhs The object to compare against this.
     *  @return @c true if the comparison holds; @c false otherwise.
     */
    /// @{
    bool operator<(const CapabilityTag& rhs) const;
    bool operator==(const CapabilityTag& rhs) const;
    bool operator!=(const CapabilityTag& rhs) const;
    /// @}
};

/**
 * Write a @c CapabilityMessageIdentifier value to an @c ostream as a string.
 *
 * @param stream The stream to write the value to.
 * @param identifier The @c CapabilityMessageIdentifier value to be written.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const CapabilityTag& identifier) {
    stream << "{endpointId:" << identifier.endpointId << ",namespace:" << identifier.nameSpace
           << ",name:" << identifier.name;
    if (identifier.instance.hasValue()) {
        stream << ",instance:" << identifier.instance.value();
    }
    stream << "}";
    return stream;
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

/**
 * @ std::hash() specialization defined to allow @c CapabilityMessageIdentifier to be used as a key in @c
 * std::unordered_map.
 */
template <>
struct hash<alexaClientSDK::avsCommon::avs::CapabilityTag> {
    size_t operator()(const alexaClientSDK::avsCommon::avs::CapabilityTag& in) const;
};

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_CAPABILITYTAG_H_
