/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGINTERFACE_H_
#define ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGINTERFACE_H_

#include <memory>
#include <string>

#include "MessageListenerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {

/**
 * Messaging interface for sending and receiving messages with arbitrary sinks
 */
class MessagingInterface {
public:
    /**
     * Write a message into a sink
     *
     * @param payload an arbitrary string
     */
    virtual void writeMessage(const std::string& payload) = 0;

    /**
     * Set a listener interface that is called when new messages are received
     * @note Subsequent call to this method override the previous listener.
     * @param messageListener
     */
    virtual void setMessageListener(std::shared_ptr<MessageListenerInterface> messageListener) = 0;
};

}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGINTERFACE_H_
