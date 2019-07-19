/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_GUARDEDVALUE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_GUARDEDVALUE_H_

#include <mutex>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Auxiliary class that uses mutex to synchronize non-trivially copyable object manipulation.
 */
template <typename ValueT>
class GuardedValue {
public:
    /**
     * Covert this object to value T.
     */
    operator ValueT() const;

    /**
     * Assign the underlying @c m_value to the new value.
     *
     * @param value The new value to be used.
     */
    ValueT operator=(const ValueT& value);

    /**
     * Builds the object with the given value.
     *
     * @param value The value to be used to initialize @c m_value.
     */
    GuardedValue(const ValueT& value);

private:
    /// The mutex used to guard access to @c m_value
    mutable std::mutex m_mutex;

    /// The actual value.
    ValueT m_value;
};

template <typename ValueT>
ValueT GuardedValue<ValueT>::operator=(const ValueT& value) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_value = value;
    return m_value;
}

template <typename ValueT>
GuardedValue<ValueT>::operator ValueT() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_value;
}

template <typename ValueT>
GuardedValue<ValueT>::GuardedValue(const ValueT& value) : m_value{value} {
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_GUARDEDVALUE_H_
