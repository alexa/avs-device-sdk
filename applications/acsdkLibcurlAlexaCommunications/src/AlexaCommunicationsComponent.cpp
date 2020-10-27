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

#include <acsdkCore/CoreComponent.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkShared/SharedComponent.h>

#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/MessageRouter.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>

#include "acsdkAlexaCommunications/AlexaCommunicationsComponent.h"

namespace alexaClientSDK {
namespace acsdkAlexaCommunications {

using namespace acl;
using namespace acsdkManufactory;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::libcurlUtils;

AlexaCommunicationsComponent getComponent() {
    return ComponentAccumulator<>()
        .addComponent(acsdkCore::getComponent())
        .addComponent(acsdkShared::getComponent())
        .addRetainedFactory(AVSConnectionManager::createAVSConnectionManagerInterface)
        .addRetainedFactory(AVSConnectionManager::createMessageSenderInterface)
        .addRetainedFactory(HTTP2TransportFactory::createTransportFactoryInterface)
        .addRetainedFactory(LibcurlHTTP2ConnectionFactory::createHTTP2ConnectionFactoryInterface)
        .addRetainedFactory(MessageRouter::createMessageRouterInterface)
        .addRetainedFactory(PostConnectSequencerFactory::createPostConnectFactoryInterface);
}

}  // namespace acsdkAlexaCommunications
}  // namespace alexaClientSDK
