/**
 * MessageRouterInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_INTERFACE_H_

#include <memory>
#include <vector>

#include "AVSUtils/Threading/Executor.h"
#include "ACL/AuthDelegateInterface.h"

#include "ACL/MessageRequest.h"
#include "ACL/Transport/MessageRouterObserverInterface.h"
#include "ACL/Transport/TransportInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This specifies the interface to manage an actual connection over some medium to AVS.
 *
 * Implementations of this class are required to be thread-safe.
 */
class MessageRouterInterface {

public:
    /**
     * Begin the process of establishing an AVS connection.
     * If the underlying implementation is already connected, or is in the process of changing its connection state,
     * this function should do nothing.
     */
    virtual void enable() = 0;

    /**
     * Close the AVS connection.
     * If the underlying implementation is not connected, or is in the process of changing its connection state,
     * this function should do nothing.
     */
    virtual void disable() = 0;

    /**
     * Queries the status of the connection.
     * @return Returns the connection status of the underlying implementation.
     */
    virtual ConnectionStatus getConnectionStatus() = 0;

    /**
     * Send a message to AVS.
     * If the underlying implementation is not connected, or is in the process of changing its connection state,
     * this function should do nothing and return false.
     * Otherwise it should enqueue the message to be sent, and return true.
     * @param request The message to be sent to AVS.
     * @return Whether the underlying implementation successfully enqueued the message to be sent.
     */
    virtual void send(std::shared_ptr<MessageRequest> request) = 0;

    /**
     * Set the URL endpoint for the AVS connection.  Calling this function with a new value will cause the
     * current active connection to be closed, and a new one opened to the new endpoint.
     * @param avsEndpoint The URL for the new AVS endpoint.
     */
    virtual void setAVSEndpoint(const std::string& avsEndpoint) = 0;

    /**
     * Set the observer to this object.
     * @param observer An observer to this class, which will be notified when the connection status changes,
     * and when messages arrive from AVS.
     */
    virtual void setObserver(std::shared_ptr<MessageRouterObserverInterface> observer) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_H_