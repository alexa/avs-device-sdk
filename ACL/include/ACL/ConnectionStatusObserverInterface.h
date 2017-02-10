/*
* ConnectionStatusObserverInterface.h
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_

#include "ACL/Values.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This class allows a client to be notified of changes to connection status to AVS.
 */
class ConnectionStatusObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~ConnectionStatusObserverInterface() = default;

    /**
     * Called when the AVS connection state changes.
     * @status The current connection status.
     * @reason The reason the status change occurred.
     */
    virtual void onConnectionStatusChanged(const ConnectionStatus status,
                                           const ConnectionChangedReason reason) = 0;
};

} // namespace acl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_CONNECTION_STATUS_OBSERVER_INTERFACE_H_
