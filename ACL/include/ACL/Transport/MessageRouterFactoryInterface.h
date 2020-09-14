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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORYINTERFACE_H_

#include "ACL/Transport/MessageRouterInterface.h"
#include "ACL/Transport/TransportFactoryInterface.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::acl;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

/**
 * Interface for creating instances of @c MessageRouterInterface
 */
class MessageRouterFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~MessageRouterFactoryInterface() = default;

    /**
     * Create a MessageRouter.
     *
     * @param authDelegate An implementation of an AuthDelegate, which will provide valid access tokens with which
     * the MessageRouter can authorize the client to AVS.
     * @param attachmentManager The AttachmentManager, which allows ACL to write attachments received from AVS.
     * @param transportFactory Factory used to create new instances of @c TransportInterface.
     * @return A new MessageRouter object
     */
    virtual std::shared_ptr<MessageRouterInterface> createMessageRouter(
        std::shared_ptr<AuthDelegateInterface> authDelegate,
        std::shared_ptr<AttachmentManager> attachmentManager,
        std::shared_ptr<TransportFactoryInterface> transportFactory) = 0;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORYINTERFACE_H_
