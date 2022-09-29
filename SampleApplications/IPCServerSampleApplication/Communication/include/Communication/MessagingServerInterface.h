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

#ifndef ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVERINTERFACE_H_

#include <memory>
#include <string>

#include "MessagingInterface.h"
#include "MessageListenerInterface.h"
#include "MessagingServerObserverInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace communication {

/**
 * Server with a @c MessagingInterface capabilities.
 */
class MessagingServerInterface : public MessagingInterface {
public:
    /**
     * Start the implementation of the @c MessagingInterface
     * @note The implementation may choose to make this method blocking or non blocking.
     * @return @c true for success. @c False otherwise.
     */
    virtual bool start() = 0;

    /**
     * Stop the messaging implmentation
     */
    virtual void stop() = 0;

    /**
     * Is the medium ready for passing messages
     */
    virtual bool isReady() = 0;

    /**
     * Set server observer.
     * @param observer The observer which should gets notified whenever a new connection is open or a connection is
     * closed.
     */
    virtual void setObserver(const std::shared_ptr<MessagingServerObserverInterface>& observer) = 0;
};

}  // namespace communication
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_COMMUNICATION_INCLUDE_COMMUNICATION_MESSAGINGSERVERINTERFACE_H_
