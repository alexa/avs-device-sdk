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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TYPEINDEX_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TYPEINDEX_H_

#include <sstream>
#include <string>
#include <typeindex>
#include <type_traits>

#include <AVSCommon/Utils/Logger/LogEntry.h>
#include <AVSCommon/Utils/Logger/LoggerUtils.h>
#include <AVSCommon/Utils/PlatformDefinitions.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
/**
 * TypeIndex provide a sortable and hashable identity for C++ types, much like std::type_index,
 * but with optional use of RTTI.
 */
struct TypeIndex {
#ifdef ACSDK_USE_RTTI

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
    explicit TypeIndex(Value value);

    /// The value used to uniquely identify a type.
    Value m_value;
};

#ifdef ACSDK_USE_RTTI

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
    return TypeIndex{&TypeIndex::TypeSpecific<
        typename std::remove_cv<typename std::remove_reference<Type>::type>::type>::m_instance};
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

/**
 * Utility function to log a name for a given type.  This can be useful when TypeIndex<Type>::getName()
 * does not return a string that is easy to correlate with C++ type names.
 *
 * @tparam Type The type to log
 * @param name The name ot log for the type.
 */
template <typename Type>
inline void logTypeIndex(const std::string& name) {
    avsCommon::utils::logger::acsdkInfo(avsCommon::utils::logger::LogEntry("TypeIndex", __func__)
                                            .d("name", name)
                                            .d("TypeIndex", getTypeIndex<Type>().getName()));
}

/**
 * Helper macro for invoking logTypeIndex<Type> without specifying the type twice.
 */
#define ACSDK_LOG_TYPE_INDEX(type) logTypeIndex<type>(#type)

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

namespace std {

template <>
struct hash<alexaClientSDK::avsCommon::utils::TypeIndex> {
    std::size_t operator()(alexaClientSDK::avsCommon::utils::TypeIndex typeIndex) const;
};

inline std::size_t hash<alexaClientSDK::avsCommon::utils::TypeIndex>::operator()(
    alexaClientSDK::avsCommon::utils::TypeIndex typeIndex) const {
    return hash<alexaClientSDK::avsCommon::utils::TypeIndex::Value>()(typeIndex.m_value);
}

}  // namespace std

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_TYPEINDEX_H_
