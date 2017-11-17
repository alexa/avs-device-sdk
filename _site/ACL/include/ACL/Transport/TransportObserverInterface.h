/*
 * TransportObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORT_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORT_OBSERVER_INTERFACE_H_

#include <memory>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>

namespace alexaClientSDK {
namespace acl {

/**
 * An interface class which allows a derived class to observe a Transport implementation.
 */
class TransportObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~TransportObserverInterface() = default;

    /**
     * Called when a connection to AVS is established.
     */
    virtual void onConnected() = 0;

    /**
     * Called when we disconnect from AVS.
     * @param reason The reason that we disconnected.
     */
    virtual void onDisconnected(avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) = 0;

    /**
     * Called when the server asks the client to reconnect
     */
    virtual void onServerSideDisconnect() = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORT_OBSERVER_INTERFACE_H_
