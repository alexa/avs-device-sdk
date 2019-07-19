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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSESINKINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSESINKINTERFACE_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "AVSCommon/Utils/HTTP2/HTTP2ResponseSinkInterface.h"
#include "AVSCommon/Utils/HTTP2/HTTP2ReceiveDataStatus.h"
#include "AVSCommon/Utils/HTTP2/HTTP2ResponseFinishedStatus.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Interface for receiving a mime encoded HTTP2 response.
 */
class HTTP2MimeResponseSinkInterface {
public:
    /**
     * Default destructor.
     */
    virtual ~HTTP2MimeResponseSinkInterface() = default;

    /**
     * Notification that an HTTP response code was returned for the request.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param responseCode The response code received for the request.
     * @return Whether receipt of the response should continue.
     */
    virtual bool onReceiveResponseCode(long responseCode) = 0;

    /**
     * Notification that an HTTP header line was received.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param line The HTTP response header line that was received.
     * @return Whether receipt of the response should continue.
     */
    virtual bool onReceiveHeaderLine(const std::string& line) = 0;

    /**
     * Notification of the start of a new mime part.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param headers A multimap from header names to header values.
     * @return Whether receipt of the response should continue.
     */
    virtual bool onBeginMimePart(const std::multimap<std::string, std::string>& headers) = 0;

    /**
     * Notification of new body data received from an HTTP2 response.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param bytes The buffer containing the bytes to consume.
     * @param size The number of bytes to consume.
     * @return Status of the operation.  @see HTTP2ReceiveDataStatus
     */
    virtual HTTP2ReceiveDataStatus onReceiveMimeData(const char* bytes, size_t size) = 0;

    /**
     * Notification of the end of the current mime part.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @return Whether receipt of the response should continue.
     */
    virtual bool onEndMimePart() = 0;

    /**
     * Notification of receipt of non-mime body data in an HTTP2 response.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param bytes The buffer containing the bytes to consume.
     * @param size The number of bytes to consume.
     * @return Status of the operation.  @see HTTP2ReceiveDataStatus.
     */
    virtual HTTP2ReceiveDataStatus onReceiveNonMimeData(const char* bytes, size_t size) = 0;

    /**
     * Notification that the request/response cycle has finished and no further notifications will be provided.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param status The status with which the response finished.  @see HTTP2ResponseFinishedStatus.
     */
    virtual void onResponseFinished(HTTP2ResponseFinishedStatus status) = 0;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2MIMERESPONSESINKINTERFACE_H_
