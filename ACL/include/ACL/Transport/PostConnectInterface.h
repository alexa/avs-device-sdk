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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTINTERFACE_H_

#include <memory>

#include "ACL/Transport/PostConnectObserverInterface.h"
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>

namespace alexaClientSDK {
namespace acl {

/**
 * Interface for post-connect objects which should be used to perform activities after a connection is established.
 */
class PostConnectInterface {
public:
    /**
     * The main method which is responsible for doing the PostConnect action.
     *
     * @note: This method is not expected to be called twice throughout the lifecycle of the object.
     *
     * @param postConnectSender The @c MessageSenderInterface used to send post connect messages.
     * @param postConnectObserver The @c PostConnectObserverInterface to get notified on success or failure of the
     * post connect action.
     * @return A boolean to indicate that the post connect process has been successfully initiated.
     */
    virtual bool doPostConnect(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> postConnectSender,
        std::shared_ptr<PostConnectObserverInterface> postConnectObserver) = 0;

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
