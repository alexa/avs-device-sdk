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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORY_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORY_H_

#include "ACL/Transport/MessageRouter.h"
#include "ACL/Transport/MessageRouterFactoryInterface.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::acl;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

/**
 * Factory for creating MessageRouter instances that manages connection over some medium to AVS.
 */
class MessageRouterFactory : public MessageRouterFactoryInterface {
public:
    /**
     * Default constructor
     */
    MessageRouterFactory();

    /// @name MessageRouterFactoryInterface methods.
    /// @{
    std::shared_ptr<MessageRouterInterface> createMessageRouter(
        std::shared_ptr<AuthDelegateInterface> authDelegate,
        std::shared_ptr<AttachmentManager> attachmentManager,
        std::shared_ptr<TransportFactoryInterface> transportFactory) override;
    /// @}
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_MESSAGEROUTERFACTORY_H_
