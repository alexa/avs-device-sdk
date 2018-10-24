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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2SENDDATARESULT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2SENDDATARESULT_H_

#include <cstddef>

#include "AVSCommon/Utils/HTTP2/HTTP2SendStatus.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Value returned from various methods that send data, combining a status and a size.
 */
struct HTTP2SendDataResult {
    /// The status of the send data operation.  @see HTTP2SendStatus.
    HTTP2SendStatus status;

    /// The number of bytes copied.  This value should only be non-zero if @c status == @c CONTINUE.
    size_t size;

    /**
     * Construct a HTTP2SendDataResult with a status of CONTINUE and the specified size
     *
     * @param size The count of bytes sent.
     */
    explicit HTTP2SendDataResult(size_t size);

    /// Const PAUSE result.
    static const HTTP2SendDataResult PAUSE;

    /// Const COMPLETE result.
    static const HTTP2SendDataResult COMPLETE;

    /// Const ABORT result.
    static const HTTP2SendDataResult ABORT;

private:
    /**
     * Construct a HTTP2SendDataResult.
     *
     * @param status The status of the send operation.
     * @param size The count of bytes sent.
     */
    HTTP2SendDataResult(HTTP2SendStatus status, size_t size);
};

inline HTTP2SendDataResult::HTTP2SendDataResult(size_t sizeIn) : status{HTTP2SendStatus::CONTINUE}, size{sizeIn} {
}

inline HTTP2SendDataResult::HTTP2SendDataResult(HTTP2SendStatus statusIn, size_t sizeIn) :
        status{statusIn},
        size{sizeIn} {
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2SENDDATARESULT_H_
