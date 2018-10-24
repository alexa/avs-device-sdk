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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_SUCCESSRESULT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_SUCCESSRESULT_H_

#include "Result.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace error {

/**
 * Version of @c Result class assuming status to be either success or failure.
 *
 * @tparam TValue Type of the value associated with the result.
 */
template <typename TValue>
class SuccessResult : public utils::error::Result<bool, TValue> {
public:
    /**
     * Creates a succeeded result with a value.
     *
     * @param value Value to be associated with the result.
     * @return Succeeded result with a value.
     */
    inline static SuccessResult<TValue> success(TValue value);

    /**
     * Creates a failed result.
     *
     * @return Failed result.
     */
    inline static SuccessResult<TValue> failure();

    /**
     * Constructor with both success status and value provided.
     *
     * @param succeeded Success status. True for succes, false for failure.
     * @param value Value to be associated with the result.
     */
    inline SuccessResult(bool succeeded, TValue value);

    /**
     * Returns true if result status is succeeded, false otherwise.
     *
     * @return True if result status is succeeded, false otherwise.
     */
    inline bool isSucceeded();

protected:
    /**
     * Constructor with only success status provided.
     *
     * @param succeeded Success status. True for success, false for failure.
     */
    explicit inline SuccessResult(bool succeeded);
};

template <typename TValue>
SuccessResult<TValue>::SuccessResult(bool succeeded) : Result<bool, TValue>(succeeded) {
}

template <typename TValue>
SuccessResult<TValue>::SuccessResult(bool succeeded, TValue value) : Result<bool, TValue>(succeeded, value) {
}

template <typename TValue>
bool SuccessResult<TValue>::isSucceeded() {
    return Result<bool, TValue>::status();
}

template <typename TValue>
SuccessResult<TValue> SuccessResult<TValue>::failure() {
    return SuccessResult(false);
}

template <typename TValue>
SuccessResult<TValue> SuccessResult<TValue>::success(TValue value) {
    return SuccessResult<TValue>(true, value);
}

}  // namespace error
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_ERROR_SUCCESSRESULT_H_
