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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_NAMESPACEANDNAME_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_NAMESPACEANDNAME_H_

#include <string>

#include "AVSCommon/AVS/CapabilityTag.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Conjoined @c namespace and @c name values (intended for identifying sub-types of @c AVSDirective).
 *
 * @deprecated This structure is being deprecated. From now on, use @c CapabilityTag instead.
 */
class NamespaceAndName : public CapabilityTag {
public:
    /**
     * Constructor to initialize with default values.
     */
    NamespaceAndName();

    /**
     * Constructor to initialize wih specific values.
     *
     * @param nameSpaceIn The @c namespace value for this instance.
     * @param nameIn The @c name value for this instance.
     */
    NamespaceAndName(const std::string& nameSpaceIn, const std::string& nameIn);

    /**
     * Constructor used to covert @c CapabilityMessageIdentifier into a @c NamespaceAndName object.
     *
     * @param identifier The @c CapabilityMessageIdentifier to copy.
     */
    NamespaceAndName(const CapabilityTag& identifier);
};

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
