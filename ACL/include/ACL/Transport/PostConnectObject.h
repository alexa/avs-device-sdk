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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTOBJECT_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTOBJECT_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include "ACL/Transport/PostConnectObserverInterface.h"
#include "ACL/Transport/PostConnectSendMessageInterface.h"
#include "ACL/Transport/TransportInterface.h"

namespace alexaClientSDK {
namespace acl {

class HTTP2Transport;

/**
 * Base class for post-connect objects.
 */
class PostConnectObject : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Static method to initialize the ContextManager.
     *
     * @param contextManager is the context manager to initialize with.
     */
    static void init(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /**
     * This method currently creates only objects of PostConnectSynchronizer.
     *
     * @return a shared pointer to PostConnectObject.
     */
    static std::shared_ptr<PostConnectObject> create();

    /**
     * The main method which is responsible for doing the PostConnect action
     * of the specific PostConnect object type.
     *
     * @param transport The transport to which the post connect is associated..
     */
    virtual bool doPostConnect(std::shared_ptr<HTTP2Transport> transport) = 0;

    /**
     * Method to add observers for PostConnectObserverInterface.
     *
     * @param observer The observer object to add.
     */
    virtual void addObserver(std::shared_ptr<PostConnectObserverInterface> observer) = 0;

    /**
     * Method to remove observers for PostConnectObserverInterface.
     *
     * @param observer The observer object to remove.
     */
    virtual void removeObserver(std::shared_ptr<PostConnectObserverInterface> observer) = 0;

    /// static instance of the ContextManager set during intialization.
    static std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

private:
    /**
     * Method to notify observers of PostConnectObserverInterface.
     */
    virtual void notifyObservers() = 0;

protected:
    /**
     * PostConnectObject Constructor.
     */
    PostConnectObject();
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTOBJECT_H_
