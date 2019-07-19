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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_RESULT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_RESULT_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace error {

/**
 * Class representing result of an operation. The result has a status and a value.
 *
 * @tparam TStatus Type of the status
 * @tparam TValue Type of the value
 */
template <typename TStatus, typename TValue>
class Result {
public:
    /**
     * Constructor with both status and return value provided.
     *
     * @param status Status of the operation.
     * @param value Value associated with the result.
     */
    inline Result(TStatus status, TValue value);

    /**
     * Constructor with only status provided
     *
     * @param status Result status.
     */
    inline Result(TStatus status);

    /**
     * Returns result status.
     *
     * @return Result status.
     */
    inline TStatus status();

    /**
     * Returns a value associated with the result.
     *
     * @return Value associated with the result.
     */
    inline TValue& value();

private:
    /// Result status.
    TStatus m_status;

    /// Value associated with the result.
    TValue m_value;
};

template <typename TStatus, typename TValue>
Result<TStatus, TValue>::Result(TStatus status, TValue value) : m_status{status}, m_value(value) {
}

template <typename TStatus, typename TValue>
Result<TStatus, TValue>::Result(TStatus status) : m_status{status} {
}

template <typename TStatus, typename TValue>
TValue& Result<TStatus, TValue>::value() {
    return m_value;
}

template <typename TStatus, typename TValue>
TStatus Result<TStatus, TValue>::status() {
    return m_status;
}

}  // namespace error
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_RESULT_H_
