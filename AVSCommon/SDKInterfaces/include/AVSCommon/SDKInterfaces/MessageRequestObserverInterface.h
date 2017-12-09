/*
 * MessageRequestObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEREQUESTOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEREQUESTOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * An interface class which allows a derived class to observe a MessageRequest implementation.
 */
class MessageRequestObserverInterface {
public:
    /**
     * This enum expresses the various end-states that a send request could arrive at.
     */
    enum class Status {
        /// The message has not yet been processed for sending.
        PENDING,

        /// The message was successfully sent.
        SUCCESS,

        /// The message was successfully sent but the HTTPReponse had no content.
        SUCCESS_NO_CONTENT,

        /// The send failed because AVS was not connected.
        NOT_CONNECTED,

        /// The send failed because AVS is not synchronized.
        NOT_SYNCHRONIZED,

        /// The send failed because of timeout waiting for AVS response.
        TIMEDOUT,

        /// The send failed due to an underlying protocol error.
        PROTOCOL_ERROR,

        /// The send failed due to an internal error within ACL.
        INTERNAL_ERROR,

        /// The send failed due to an internal error on the server which sends code 500.
        SERVER_INTERNAL_ERROR_V2,

        /// The send failed due to server refusing the request.
        REFUSED,

        /// The send failed due to server canceling it before the transmission completed.
        CANCELED,

        /// The send failed due to excessive load on the server.
        THROTTLED,

        /// The access credentials provided to ACL were invalid.
        INVALID_AUTH,

        /// The send failed due to invalid request sent by the user.
        BAD_REQUEST,

        /// The send failed due to unknown server error.
        SERVER_OTHER_ERROR
    };

    /*
     * Destructor
     */
    virtual ~MessageRequestObserverInterface() = default;

    /*
     * Called when a message request has been processed by AVS.
     */
    virtual void onSendCompleted(MessageRequestObserverInterface::Status status) = 0;

    /*
     * Called when an exception is thrown when trying to send a message to AVS.
     */
    virtual void onExceptionReceived(const std::string& exceptionMessage) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGEREQUESTOBSERVERINTERFACE_H_
