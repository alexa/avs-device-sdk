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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORTFACTORY_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORTFACTORY_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/HTTP2/HTTP2ConnectionFactoryInterface.h>

#include "ACL/Transport/MessageConsumerInterface.h"
#include "ACL/Transport/PostConnectFactoryInterface.h"
#include "ACL/Transport/TransportFactoryInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

/**
 * An HTTP2TransportFactory creates HTTP2Transport objects.
 */
class HTTP2TransportFactory : public TransportFactoryInterface {
public:
    /**
     * HTTP2TransportFactory constructor.
     *
     * @param connectionFactory Object used to create instances of HTTP2ConnectionInterface.
     * @param postConnectFactory Object used to create instances of the PostConnectInterface.
     */
    HTTP2TransportFactory(
        std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionFactoryInterface> connectionFactory,
        std::shared_ptr<PostConnectFactoryInterface> postConnectFactory);

    /// @name TransportFactoryInterface methods.
    /// @{
    std::shared_ptr<TransportInterface> createTransport(
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
        const std::string& avsEndpoint,
        std::shared_ptr<MessageConsumerInterface> messageConsumerInterface,
        std::shared_ptr<TransportObserverInterface> transportObserverInterface) override;
    /// @}

    /**
     * Deleted default constructor.
     */
    HTTP2TransportFactory() = delete;

private:
    /// Save a pointer to the object used to create instances of the HTTP2ConnectionInterface.
    std::shared_ptr<avsCommon::utils::http2::HTTP2ConnectionFactoryInterface> m_connectionFactory;

    /// Save a pointer to the object used to create instances of the PostConnectInterface.
    std::shared_ptr<PostConnectFactoryInterface> m_postConnectFactory;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_HTTP2TRANSPORTFACTORY_H_
