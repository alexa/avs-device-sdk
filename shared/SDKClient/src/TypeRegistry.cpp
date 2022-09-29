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

#include "acsdk/SDKClient/internal/TypeRegistry.h"

namespace alexaClientSDK {
namespace sdkClient {
namespace internal {
void TypeRegistry::addTypeIndex(const avsCommon::utils::TypeIndex& typeIndex) {
    m_types.insert(typeIndex);
}

bool TypeRegistry::empty() const {
    return m_types.empty();
}

TypeRegistry TypeRegistry::typeDifference(const TypeRegistry& other) const {
    TypeRegistry result;
    for (const auto& type : m_types) {
        if (!other.m_types.count(type)) {
            result.m_types.insert(type);
        }
    }
    return result;
}

bool TypeRegistry::typeDifferenceIsEmpty(const TypeRegistry& other) const {
    for (const auto& type : m_types) {
        if (!other.m_types.count(type)) {
            return false;
        }
    }
    return true;
}

void TypeRegistry::outputToStream(std::ostream& stream) const {
    stream << "[";
    for (auto it = m_types.begin(); it != m_types.end(); ++it) {
        if (it != m_types.begin()) {
            stream << ", ";
        }
        stream << it->getName();
    }
    stream << "]";
}

std::string TypeRegistry::toString() const {
    std::stringstream ss;
    outputToStream(ss);
    return ss.str();
}

TypeRegistry::const_iterator TypeRegistry::cbegin() const {
    return m_types.cbegin();
}

TypeRegistry::const_iterator TypeRegistry::cend() const {
    return m_types.cend();
}
}  // namespace internal
}  // namespace sdkClient
}  // namespace alexaClientSDK