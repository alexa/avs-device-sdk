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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGESENDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGESENDERINTERFACE_H_

#include "AVSCommon/AVS/MessageRequest.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/// This specifies an interface to send a message.
class MessageSenderInterface {
public:
    /// Destructor.
    virtual ~MessageSenderInterface() = default;

    /**
     * Send a message.  This function must operate asynchronously, internally storing the message to be sent
     * when the connection is able to process it.
     * The @c onSendCompleted callback method of the @c MessageRequest object is invoked as follows:
     *
     * @li If the @c MessageSenderInterface object is not connected, the callback value is NOT_CONNECTED.
     * @li Otherwise, the object attempts to send the message, and once this has either succeeded or failed,
     *     the callback value is set appropriately.
     *
     * @param request The @c MessageRequest to send.
     */
    virtual void sendMessage(std::shared_ptr<avs::MessageRequest> request) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGESENDERINTERFACE_H_
