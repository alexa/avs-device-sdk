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
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "ACL/Transport/HTTP2TransportFactory.h"
#include "ACL/Transport/HTTP2Transport.h"
#include "ACL/Transport/PostConnectSynchronizer.h"

namespace alexaClientSDK {
namespace acl {

using namespace alexaClientSDK::avsCommon::utils;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::avs::attachment;

/// Key for the root node value containing configuration values for ACL.
static const std::string ACL_CONFIG_KEY = "acl";
/// Key for the 'endpoint' value under the @c ACL_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY = "endpoint";
/// Default @c AVS endpoint to connect to.
static const std::string DEFAULT_AVS_ENDPOINT = "https://avs-alexa-na.amazon.com";

std::shared_ptr<TransportInterface> HTTP2TransportFactory::createTransport(
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    std::shared_ptr<AttachmentManager> attachmentManager,
    const std::string& avsEndpoint,
    std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
    std::shared_ptr<TransportObserverInterface> transportObserverInterface) {
    auto connection = m_connectionFactory->createHTTP2Connection();
    if (!connection) {
        return nullptr;
    }

    std::string configuredEndpoint = avsEndpoint;
    if (configuredEndpoint.empty()) {
        alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[ACL_CONFIG_KEY].getString(
            ENDPOINT_KEY, &configuredEndpoint, DEFAULT_AVS_ENDPOINT);
    }
    return HTTP2Transport::create(
        authDelegate,
        configuredEndpoint,
        connection,
        messageConsumerInterface,
        attachmentManager,
        transportObserverInterface,
        m_postConnectFactory);
}

HTTP2TransportFactory::HTTP2TransportFactory(
    std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionFactoryInterface> connectionFactory,
    std::shared_ptr<PostConnectFactoryInterface> postConnectFactory) :
        m_connectionFactory{connectionFactory},
        m_postConnectFactory{postConnectFactory} {
}

}  // namespace acl
}  // namespace alexaClientSDK
