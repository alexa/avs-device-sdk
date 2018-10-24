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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTSOURCEINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTSOURCEINTERFACE_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "AVSCommon/Utils/HTTP2/HTTP2GetMimeHeadersResult.h"
#include "AVSCommon/Utils/HTTP2/HTTP2SendDataResult.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Interface for providing data to be sent as part of a Mime encoded HTTP2 request.
 */
class HTTP2MimeRequestSourceInterface {
public:
    /**
     * Default destructor.
     */
    virtual ~HTTP2MimeRequestSourceInterface() = default;

    /**
     * Get the header lines that should be output with this HTTP2 request.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @return The header lines that should be output with this request.
     */
    virtual std::vector<std::string> getRequestHeaderLines() = 0;

    /**
     * Get the header lines that should be output with the next mime part.  This will be called once before
     * @c onSendMimePartData() is called for the first mime part and after each call to @c onSendMimePartData()
     * that returns @c HTTP2SendDataResult.status == COMPLETE.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @return An @c HTTP2GetMimeHeadersResult specifying the status of the operation and a vector of header lines
     * if the status was CONTINUE.
     */
    virtual HTTP2GetMimeHeadersResult getMimePartHeaderLines() = 0;

    /**
     * Notification to copy data to be mime encoded in to an HTTP2 request.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param bytes The buffer to receive the bytes to send.
     * @param size The max number of bytes to copy.
     * @return Result indicating the disposition of the operation and number of bytes copied.
     * @see HTTPSendMimePartDataResult.
     */
    virtual HTTP2SendDataResult onSendMimePartData(char* bytes, size_t size) = 0;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMEREQUESTSOURCEINTERFACE_H_
