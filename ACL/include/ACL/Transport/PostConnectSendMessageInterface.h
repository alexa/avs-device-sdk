/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSENDMESSAGEINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSENDMESSAGEINTERFACE_H_

#include "AVSCommon/AVS/MessageRequest.h"

namespace alexaClientSDK {
namespace acl {

/*
 * This specifies an interface to send a message even when connection to AVS
 * is not fully established.
 */
class PostConnectSendMessageInterface {
public:
    /**
     * Destructor.
     */
    virtual ~PostConnectSendMessageInterface() = default;

    /**
     * Send a message even when the transport is not connected.
     * This function must operate asynchronously, internally queuing the
     * message to be sent until the connection is able to process it.
     * The onSendCompleted callback method of the MessageRequest object is invoked as follows:
     *
     * The object attempts to send the message, and once this has either
     * succeeded or failed, the callback value is set appropriately.
     *
     * @param request MessageRequest to send a post connect message.
     */
    virtual void sendPostConnectMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSENDMESSAGEINTERFACE_H_
