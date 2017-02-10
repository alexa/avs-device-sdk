/*
* MessageRouterObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_OBSERVER_INTERFACE_H_

#include "ACL/Values.h"
#include "ACL/Message.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This interface class allows notifications from a MessageRouter object (or any derived class),
 * while not adding to the observer's public interface.  This is achieved by a friend relationship.
 * The MessageRouterObserverInterface will be notified when either the connection status changes,
 * or when a message arrives from AVS.
 */
class MessageRouterObserverInterface {
private:
    /**
     * This function will be called when the connection status changes.
     * @param status The current status of the connection.
     * @param reason The reason the connection status changed.
     */
    virtual void onConnectionStatusChanged(const ConnectionStatus status,
                                           const ConnectionChangedReason reason) = 0;

    /**
     * This function will be called when a Message arrives from AVS.
     * @param msg The message that was received from AVS.
     */
    virtual void receive(std::shared_ptr<Message> msg) = 0;

    /// The friend declaration.
    friend class MessageRouter;
};

} // namespace ACL
} // namespace AlexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGE_ROUTER_OBSERVER_INTERFACE_H_
