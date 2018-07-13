/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_NAMESPACEANDNAME_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_NAMESPACEANDNAME_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Conjoined @c namespace and @c name values (intended for identifying sub-types of @c AVSDirective).
 */
class NamespaceAndName {
public:
    /**
     * Constructor to initialize with default values.
     */
    NamespaceAndName() = default;

    /**
     * Constructor to initialize wih specific values.
     *
     * @param nameSpaceIn The @c namespace value for this instance.
     * @param nameIn The @c name value for this instance.
     */
    NamespaceAndName(const std::string& nameSpaceIn, const std::string& nameIn);

    /// The @c namespace value of this instance.
    const std::string nameSpace;

    /// The @c name value of this instance.
    const std::string name;
};

/**
 * Operator == to allow @c namespaceAndName ot be used as a key in @cstd::unordered_map.
 *
 * @param rhs The left hand side of the == operation.
 * @param rhs The right hand side of the == operation.
 * @return Whether or not this instance and @c rhs are equivalent.
 */
bool operator==(const NamespaceAndName& lhs, const NamespaceAndName& rhs);

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

/**
 * @ std::hash() specialization defined to allow @c NamespaceAndName to be used as a key in @c std::unordered_map.
 */
template <>
struct hash<alexaClientSDK::avsCommon::avs::NamespaceAndName> {
    size_t operator()(const alexaClientSDK::avsCommon::avs::NamespaceAndName& in) const;
};

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_NAMESPACEANDNAME_H_
