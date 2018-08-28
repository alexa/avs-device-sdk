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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORTFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORTFACTORYINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

#include "ACL/Transport/TransportInterface.h"
#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * This is the interface for the transport factory
 */
class TransportFactoryInterface {
public:
    /**
     * Creates a new transport.
     *
     * @param authDelegate The AuthDelegateInterface to use for authentication and authorization with AVS.
     * @param avsEndpoint The URL for the AVS server we will connect to.
     * @param messageConsumerInterface The object which should be notified on messages which arrive from AVS.
     * @param transportObserverInterface A pointer to the transport observer the new transport should notify.
     * @return A new MessageRouter object.
     */
    virtual std::shared_ptr<TransportInterface> createTransport(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        const std::string& avsEndpoint,
        std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
        std::shared_ptr<TransportObserverInterface> transportObserverInterface) = 0;

    /**
     * Destructor.
     */
    virtual ~TransportFactoryInterface() = default;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_TRANSPORTFACTORYINTERFACE_H_
