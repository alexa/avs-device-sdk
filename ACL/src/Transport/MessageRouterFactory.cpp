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

#include "ACL/Transport/MessageRouterFactory.h"

#include <memory>

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::acl;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

MessageRouterFactory::MessageRouterFactory() = default;

std::shared_ptr<MessageRouterInterface> MessageRouterFactory::createMessageRouter(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    std::shared_ptr<AttachmentManager> attachmentManager,
    std::shared_ptr<TransportFactoryInterface> transportFactory) {
    return std::make_shared<MessageRouter>(
        std::move(authDelegate), std::move(attachmentManager), std::move(transportFactory));
}

}  // namespace acl
}  // namespace alexaClientSDK
