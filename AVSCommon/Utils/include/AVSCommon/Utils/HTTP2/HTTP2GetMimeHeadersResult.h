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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2GETMIMEHEADERSRESULT_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2GETMIMEHEADERSRESULT_H_

#include <string>
#include <vector>

#include "AVSCommon/Utils/HTTP2/HTTP2SendStatus.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Value returned from @c HTTP2MimeRequestSourceInterface::getMimeHeaderLines(), combining a status and a
 * vector of header lines.
 */
struct HTTP2GetMimeHeadersResult {
public:
    /// The status of the @c getMimeHeadersLines() operation.  @see HTTP2SendStatus.
    HTTP2SendStatus status;

    /// The headers returned from @c getMimeHeaderLines.  Only non-empty if @c status == @c CONTINUE.
    std::vector<std::string> headers;

    /**
     * Construct a HTTP2GetMimeHeadersResult with a status of CONTINUE and the header values to continue with.
     * @param headers The headers to send.
     * @return A HTTP2GetMimeHeadersResult with status CONTINUE and the specified header lines.
     */
    HTTP2GetMimeHeadersResult(const std::vector<std::string>& headers);

    /// Const PAUSE result.
    static const HTTP2GetMimeHeadersResult PAUSE;

    /// Const COMPLETE result.
    static const HTTP2GetMimeHeadersResult COMPLETE;

    /// Const ABORT result.
    static const HTTP2GetMimeHeadersResult ABORT;

private:
    /**
     * Constructor.
     *
     * @param status The status of the @c getMimeHeaders() operation.
     * @param headers The headers to send.
     */
    HTTP2GetMimeHeadersResult(HTTP2SendStatus status, const std::vector<std::string>& headers);
};

inline HTTP2GetMimeHeadersResult::HTTP2GetMimeHeadersResult(const std::vector<std::string>& headers) :
        status{HTTP2SendStatus::CONTINUE},
        headers{headers} {
}

inline HTTP2GetMimeHeadersResult::HTTP2GetMimeHeadersResult(
    HTTP2SendStatus statusIn,
    const std::vector<std::string>& headersIn) :
        status{statusIn},
        headers{headersIn} {
}

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2GETMIMEHEADERSRESULT_H_
