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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_COMPONENTCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_COMPONENTCONFIGURATION_H_

#include <functional>
#include <memory>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * struct that represents the Component version information.
 */
struct ComponentConfiguration {
    /**
     * Creates a @c ComponentConfiguration
     *
     * @param name The name of the component.
     * @param version The version of the component.
     *
     * @return Pointer to @c ComponentConfiguration. Null pointer if configuration is invalid.
     */
    static std::shared_ptr<ComponentConfiguration> createComponentConfiguration(std::string name, std::string version);

    /// string indicating the name of a @c Component.
    std::string name;
    /// string indicating the version of a @c Component.
    std::string version;

private:
    /**
     * The Constructor
     *
     * @param name The name of the component
     * @param version The version of the component
     **/
    ComponentConfiguration(std::string name, std::string version);
};

/**
 * Operator == for @c ComponentConfiguration
 *
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const ComponentConfiguration& lhs, const ComponentConfiguration& rhs);

/**
 * Operator != for @c ComponentConfiguration
 *
 * @param lhs The left hand side of the != operation.
 * @param rhs The right hand side of the != operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(const ComponentConfiguration& lhs, const ComponentConfiguration& rhs);

/**
 * Operator == for shared_ptr of @c ComponentConfiguration
 *
 * @param lhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const std::shared_ptr<ComponentConfiguration> lhs, const std::shared_ptr<ComponentConfiguration> rhs);

/**
 * Operator != for shared_ptr of @c ComponentConfiguration
 *
 * @param lhs The left hand side of the != operation.
 * @param rhs The right hand side of the != operation.
 * @return Whether or not this instance and @c rhs are not equivalent.
 */
bool operator!=(const std::shared_ptr<ComponentConfiguration> lhs, const std::shared_ptr<ComponentConfiguration> rhs);

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

/**
 * @ std::hash() specialization defined for @c ComponentConfiguration. The hash is only based on the name of the
 * component configuration and does not take into account the version.
 */
template <>
struct hash<alexaClientSDK::avsCommon::avs::ComponentConfiguration> {
    size_t operator()(const alexaClientSDK::avsCommon::avs::ComponentConfiguration& in) const;
};

template <>
struct hash<std::shared_ptr<alexaClientSDK::avsCommon::avs::ComponentConfiguration>> {
    size_t operator()(const std::shared_ptr<alexaClientSDK::avsCommon::avs::ComponentConfiguration> in) const;
};

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_COMPONENTCONFIGURATION_H_
