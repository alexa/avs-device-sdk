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

#include <tuple>

#include "AVSCommon/Utils/functional/hash.h"
#include "AVSCommon/AVS/NamespaceAndName.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

NamespaceAndName::NamespaceAndName(const std::string& nameSpaceIn, const std::string& nameIn) :
        nameSpace{nameSpaceIn},
        name{nameIn} {
}

bool operator==(const NamespaceAndName& lhs, const NamespaceAndName& rhs) {
    return std::tie(lhs.nameSpace, lhs.name) == std::tie(rhs.nameSpace, rhs.name);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

size_t hash<alexaClientSDK::avsCommon::avs::NamespaceAndName>::operator()(
    const alexaClientSDK::avsCommon::avs::NamespaceAndName& in) const {
    std::size_t seed = 0;
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.nameSpace);
    alexaClientSDK::avsCommon::utils::functional::hashCombine(seed, in.name);
    return seed;
};

}  // namespace std
