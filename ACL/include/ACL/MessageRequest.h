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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_REQUEST_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_REQUEST_H_

#include "ACL/Message.h"
#include "ACL/Values.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This is a wrapper class which allows a client to send a Message to AVS, and be notified when the attempt to
 * send the Message was completed.
 */
class MessageRequest {
public:
    /**
     * Constructor.
     * @param msg The Message object to be sent to AVS.
     */
    MessageRequest(std::shared_ptr<Message> message);

    /**
     * Destructor.
     */
    virtual ~MessageRequest();

    /**
     * Retrieves the Message object to be sent to AVS.
     * @return The Message object to be sent to AVS.
     */
    std::shared_ptr<Message> getMessage();

    /**
     * This is called once the send request has completed.  The status parameter indicates success or failure.
     * @param status Whether the send request succeeded or failed.
     */
    virtual void onSendCompleted(SendMessageStatus status);

private:

    /// The Message object to be sent to AVS.
    std::shared_ptr<Message> m_message;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_MESSAGE_REQUEST_H_