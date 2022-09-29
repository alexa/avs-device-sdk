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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_TYPEREGISTRY_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_TYPEREGISTRY_H_

#include <unordered_set>

#include <AVSCommon/Utils/PlatformDefinitions.h>
#include <AVSCommon/Utils/TypeIndex.h>

namespace alexaClientSDK {
namespace sdkClient {
namespace internal {
/**
 * Internal class which keeps track of a set of types
 */
class TypeRegistry {
public:
    /**
     * Adds the given type, provided as a template to the registry
     * @tparam ComponentType The type to add
     */
    template <typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline void addType() {
        m_types.insert(avsCommon::utils::getTypeIndex<ComponentType>());
    }

    /**
     * Adds the given type, provided as a parameter to the registry
     * @param typeIndex The type to add
     */
    void addTypeIndex(const avsCommon::utils::TypeIndex& typeIndex);

    /**
     * Removes the given type from the registry
     * @tparam ComponentType The type to remove
     */
    template <typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline void removeType() {
        m_types.erase(avsCommon::utils::getTypeIndex<ComponentType>());
    }

    /**
     * Check if this registry contains no items
     * @return true if the registry is empty
     */
    bool empty() const;

    /**
     * Returns a type registry containing the types present in this TypeRegistry, but not the other
     * @param other The other TypeRegistry
     * @return A TypeRegistry containing the types present in this but not other
     */
    TypeRegistry typeDifference(const TypeRegistry& other) const;

    /**
     * Returns whether all types present in this registry are present in the other
     * @param other The other TypeRegistry
     * @return true if all types present in this are also in other
     */
    bool typeDifferenceIsEmpty(const TypeRegistry& other) const;

    /**
     * Uses the given stream to output a list of types contained within this registry, note that the readability of the
     * string depends to a large extent on whether the SDK has been built with RTTI support.
     * @param stream The output stream
     */
    void outputToStream(std::ostream& stream) const;

    /**
     * Creates a string containing the list of types contained within this registry, note that the readability of the
     * string depends to a large extent on whether the SDK has been built with RTTI support.
     * @return A string of the format "[type1, type2, type3, ...]"
     */
    std::string toString() const;

    /// type alias for const_iterator, no order is guaranteed
    using const_iterator = std::unordered_set<avsCommon::utils::TypeIndex>::const_iterator;

    /**
     * Get the const_iterator referencing the first of the types in this TypeRegistry
     * @return const_iterator to the first item
     */
    const_iterator cbegin() const;

    /**
     * Get the const_iterator referencing one past the end of the types in this TypeRegistry
     * @return const_iterator to the first item
     */
    const_iterator cend() const;

private:
    /// The set of types registered to this TypeRegistry
    std::unordered_set<avsCommon::utils::TypeIndex> m_types;
};

/**
 * Write a @c TypeRegistry to an @c ostream.
 *
 * @param stream The stream to write the value to.
 * @param typeRegistry The @c TypeRegistry to write to the @c ostream as a string.
 * @return The @c ostream that was passed in and written to.
 */
inline std::ostream& operator<<(std::ostream& stream, const TypeRegistry& typeRegistry) {
    typeRegistry.outputToStream(stream);
    return stream;
}
}  // namespace internal
}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_TYPEREGISTRY_H_
