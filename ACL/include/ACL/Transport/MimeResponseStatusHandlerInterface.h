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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESTATUSHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESTATUSHANDLERINTERFACE_H_

#include <string>

#include <AVSCommon/Utils/HTTP2/HTTP2ResponseFinishedStatus.h>

namespace alexaClientSDK {
namespace acl {

/**
 * Handle HTTP response codes and finished notifications for mime encoded responses from AVS.
 */
class MimeResponseStatusHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MimeResponseStatusHandlerInterface() = default;

    /**
     * Notification of network activity between this client and AVS.
     * (this is used to detect sustained inactivity requiring the send of a ping).
     */
    virtual void onActivity() = 0;

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
     * Notification that the request/response cycle has finished and no further notifications will be provided.
     *
     * @note Calls to this method may block network operations for the associated instance of HTTP2ConnectionInterface,
     * so they should return quickly.
     *
     * @param status The status included in the response.
     * @param nonMimeBody The body of the reply (for non HTTPResponseCode::SUCCESS_OK responses)
     */
    virtual void onResponseFinished(
        avsCommon::utils::http2::HTTP2ResponseFinishedStatus status,
        const std::string& nonMimeBody) = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MIMERESPONSESTATUSHANDLERINTERFACE_H_
