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

#include <AVSCommon/Utils/Threading/Executor.h>

#include "ACL/Transport/HTTP2TransportFactory.h"
#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/PostConnectSynchronizer.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

std::shared_ptr<TransportInterface> HTTP2TransportFactory::createTransport(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    std::shared_ptr<AttachmentManager> attachmentManager,
    const std::string& avsEndpoint,
    std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
    std::shared_ptr<TransportObserverInterface> transportObserverInterface) {
    return HTTP2Transport::create(
        authDelegate,
        avsEndpoint,
        messageConsumerInterface,
        attachmentManager,
        transportObserverInterface,
        m_postConnectFactory);
}

HTTP2TransportFactory::HTTP2TransportFactory(std::shared_ptr<PostConnectFactoryInterface> postConnectFactory) :
        m_postConnectFactory{postConnectFactory} {
}

}  // namespace acl
}  // namespace alexaClientSDK
