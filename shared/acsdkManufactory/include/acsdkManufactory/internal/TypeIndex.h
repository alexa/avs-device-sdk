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

#ifndef ACSDKMANUFACTORY_INTERNAL_TYPEINDEX_H_
#define ACSDKMANUFACTORY_INTERNAL_TYPEINDEX_H_

#include <functional>
#include <sstream>
#include <typeindex>
#include <type_traits>
#include "acsdkManufactory/internal/TypeTraitsHelper.h"

#include <string>

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/**
 * TypeIndex provide a sortable and hashable identity for C++ types, much like std::type_index,
 * but with optional use of RTTI.
 */
struct TypeIndex {
#if ACSDK_USE_RTTI

    /// Type of value used to identify discrete types.  With RTTI enabled we can just use std::type_index.
    using Value = std::type_index;

#else  // ACSDK_USE_RTTI

    /// Type of value used to identify discrete types.  Without RTTI enabled this will point to a static
    /// member of the @c TypeSpecific<Type> class.
    using Value = void*;

    /**
     * Template class used to create type specific (static) values.  The address of these values
     * is used by TypeIndex to identify specific types.
     */
    template <typename Type>
    struct TypeSpecific {
        static char m_instance;
    };

#endif  // ACSDK_USE_RTTI

    /**
     * A string that uniquely identifies the type.
     *
     * @return A string that uniquely identifies the type.
     */
    std::string getName() const;

    /**
     * Equality operator.
     *
     * @param rhs The right hand side of the comparison.
     * @return Whether the two TypeIndex instances refer to the same type.
     */
    bool operator==(TypeIndex rhs) const;

    /**
     * Non-Equality operator.
     *
     * @param rhs The right hand side of the comparison.
     * @return Whether the two TypeIndex instances do NOT refer to the same type.
     */
    bool operator!=(TypeIndex rhs) const;

    /**
     * Comparison operator.
     *
     * @param rhs The right hand side of the comparison.
     * @return Whether this instance should sort as less than the rhs instance.
     */
    bool operator<(TypeIndex rhs) const;

private:
    friend struct std::hash<TypeIndex>;

    template <typename Type>
    friend TypeIndex getTypeIndex();

    /**
     * Constructor.
     *
     * @param value A value that uniquely identifies a type.
     */
    TypeIndex(Value value);

    /// The value used to uniquely identify a type.
    Value m_value;
};

#if ACSDK_USE_RTTI

/**
 * Get the TypeIndex value for @c Type.
 *
 * @tparam Type The type for which to create a TypeIndex value.
 */
template <typename Type>
inline TypeIndex getTypeIndex() {
    return TypeIndex{typeid(Type)};
}

inline std::string TypeIndex::getName() const {
    return m_value.name();
}

#else  // ACSDK_USE_RTTI

template <typename Type>
char TypeIndex::TypeSpecific<Type>::m_instance;

/**
 * Get the TypeIndex value for @c Type.
 *
 * @tparam Type The type for which to create a TypeIndex value.
 */
template <typename Type>
inline TypeIndex getTypeIndex() {
    return TypeIndex{&TypeIndex::TypeSpecific<RemoveCvref_t<Type> >::m_instance};
}

inline std::string TypeIndex::getName() const {
    std::stringstream ss;
    ss << '[' << m_value << ']';
    return ss.str();
}

#endif  // ACSDK_USE_RTTI

inline TypeIndex::TypeIndex(Value value) : m_value{value} {
}

inline bool TypeIndex::operator==(TypeIndex x) const {
    return m_value == x.m_value;
}

inline bool TypeIndex::operator!=(TypeIndex x) const {
    return m_value != x.m_value;
}

inline bool TypeIndex::operator<(TypeIndex x) const {
    return m_value < x.m_value;
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

namespace std {

template <>
struct hash<alexaClientSDK::acsdkManufactory::internal::TypeIndex> {
    std::size_t operator()(alexaClientSDK::acsdkManufactory::internal::TypeIndex typeIndex) const;
};

inline std::size_t hash<alexaClientSDK::acsdkManufactory::internal::TypeIndex>::operator()(
    alexaClientSDK::acsdkManufactory::internal::TypeIndex typeIndex) const {
    return hash<alexaClientSDK::acsdkManufactory::internal::TypeIndex::Value>()(typeIndex.m_value);
}

}  // namespace std

#endif  // ACSDKMANUFACTORY_INTERNAL_TYPEINDEX_H_