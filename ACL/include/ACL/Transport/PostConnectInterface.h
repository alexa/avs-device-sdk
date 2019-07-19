/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTINTERFACE_H_

#include <memory>

#include "ACL/Transport/PostConnectObserverInterface.h"
#include "ACL/Transport/PostConnectSendMessageInterface.h"

namespace alexaClientSDK {
namespace acl {

class HTTP2Transport;

/**
 * Interface for post-connect objects which should be used to perform activities after a connection is established.
 */
class PostConnectInterface {
public:
    /**
     * The main method which is responsible for doing the PostConnect action
     * of the specific PostConnect object type.
     *
     * @param transport The transport to which the post connect is associated..
     *
     * @return A boolean to indicate that the post connect process has been successfully initiated
     */
    virtual bool doPostConnect(std::shared_ptr<HTTP2Transport> transport) = 0;

    /**
     * Handle notification that the connection has been lost.
     */
    virtual void onDisconnect() = 0;

    /**
     * PostConnectInterface destructor
     */
    virtual ~PostConnectInterface() = default;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTINTERFACE_H_
