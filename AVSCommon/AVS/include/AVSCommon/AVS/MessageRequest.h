/*
 * MessageRequest.h
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_MESSAGE_REQUEST_H_
#define ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_MESSAGE_REQUEST_H_

#include <memory>
#include <string>

#include "AVSCommon/AVS/Attachment/AttachmentReader.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * This is a wrapper class which allows a client to send a Message to AVS, and be notified when the attempt to
 * send the Message was completed.
 */
class MessageRequest {
public:
    /**
     * This enum expresses the various end-states that a send request could arrive at.
     */
    enum class Status {
        /// The message has not yet been processed for sending.
        PENDING,

        /// The message was successfully sent.
        SUCCESS,

        /// The send failed because AVS was not connected.
        NOT_CONNECTED,

        /// The send failed because of timeout waiting for AVS response.
        TIMEDOUT,

        /// The send failed due to an underlying protocol error.
        PROTOCOL_ERROR,

        /// The send failed due to an internal error within ACL.
        INTERNAL_ERROR,

        /// The send failed due to an internal error on the server.
        SERVER_INTERNAL_ERROR,

        /// The send failed due to server refusing the request.
        REFUSED,

        /// The send failed due to server canceling it before the transmission completed.
        CANCELED,

        /// The send failed due to excessive load on the server.
        THROTTLED,

        /// The access credentials provided to ACL were invalid.
        INVALID_AUTH
    };

    /**
     * Constructor.
     * @param jsonContent The message to be sent to AVS.
     * @param attachmentReader The attachment data (if present) to be sent to AVS along with the message.
     * Defaults to @c nullptr.
     */
    MessageRequest(const std::string & jsonContent,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> attachmentReader = nullptr);

    /**
     * Destructor.
     */
    virtual ~MessageRequest();

    /**
     * Retrieves the JSON content to be sent to AVS.
     *
     * @return The JSON content to be sent to AVS.
     */
    std::string getJsonContent();

    /**
     * Retrieves the AttachmentReader of the Attachment data to be sent to AVS.
     *
     * @return The AttachmentReader of the Attachment data to be sent to AVS.
     */
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> getAttachmentReader();

    /**
     * This is called once the send request has completed.  The status parameter indicates success or failure.
     * @param status Whether the send request succeeded or failed.
     */
    virtual void onSendCompleted(Status status);

    /**
     * This function will be called if AVS responds with an exception message to this message request being sent.
     *
     * @param exceptionMessage The exception message.
     */
    virtual void onExceptionReceived(const std::string & exceptionMessage);

    /**
     * Utility function to convert a modern enum class to a string.
     *
     * @param status The enum value.
     * @return The string representation of the incoming value.
     */
    static std::string statusToString(Status status);

private:
    /// The JSON content to be sent to AVS.
    std::string m_jsonContent;
    /// The AttachmentReader of the Attachment data to be sent to AVS.
    std::shared_ptr<avsCommon::avs::attachment::AttachmentReader> m_attachmentReader;
};

} // namespace avs
} // namespace avsCommon
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_AVS_COMMON_AVS_INCLUDE_AVS_COMMON_AVS_MESSAGE_REQUEST_H_
