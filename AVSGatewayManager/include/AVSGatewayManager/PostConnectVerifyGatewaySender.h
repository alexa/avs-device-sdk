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

#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_POSTCONNECTVERIFYGATEWAYSENDER_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_POSTCONNECTVERIFYGATEWAYSENDER_H_

#include <memory>
#include <mutex>
#include <functional>

#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>
#include <AVSCommon/AVS/PostConnectMessageRequest.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <AVSGatewayManager/GatewayVerifyState.h>

namespace alexaClientSDK {
namespace avsGatewayManager {

/**
 * The post connect operation which sends the @c VerifyGateway event.
 */
class PostConnectVerifyGatewaySender
        : public avsCommon::sdkInterfaces::PostConnectOperationInterface
        , public std::enable_shared_from_this<PostConnectVerifyGatewaySender> {
public:
    /**
     * Creates a new instance of @c PostConnectVerifyGatewaySender.
     *
     * @param gatewayVerifiedCallback The callback method that should be called on successful gateway verification.
     * @return a new instance of the @c PostConnectVerifyGatewaySender.
     */
    static std::shared_ptr<PostConnectVerifyGatewaySender> create(
        std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>&)> gatewayVerifiedCallback);

    /// @name PostConnectOperationInterface Methods
    /// @{
    unsigned int getOperationPriority() override;
    bool performOperation(const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>&
                              postConnectMessageSender) override;
    void abortOperation() override;
    /// @}
private:
    /**
     * Return codes for the @c sendVerifyGateway method.
     */
    enum class VerifyGatewayReturnCode {
        /// The AVS gateway has been verified.
        GATEWAY_VERIFIED,
        /// The @c VerifyGateway event received a 200 response with a SetGateway Directive gateways should be changed.
        CHANGING_GATEWAY,
        /// The @c VerifyGateway event received a fatal error response.
        FATAL_ERROR,
        /// The @c VerifyGateway event received a retriable error response.
        RETRIABLE_ERROR
    };

    /**
     * Constructor.
     *
     * @param gatewayVerifiedCallback The callback method that should be called on successful gateway verification.
     */
    PostConnectVerifyGatewaySender(
        std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>&)> gatewayVerifiedCallback);

    /**
     * The VerifyGateway operation which sends the @c ApiGateway.VerifyGateway event.
     *
     * @param postConnectMessageSender The @c PostConnectMessageSender to send the post connect message.
     * @return The @c VerifyGatewayReturnCode used to determine the next action.
     */
    VerifyGatewayReturnCode sendVerifyGateway(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectSendMessageInterface>& postConnectMessageSender);

    /**
     * Thread safe method to check if the operation is stopping.
     * @return True if the operation is stopping, else false.
     */
    bool isStopping();

    /// The Callback function that will be called after successful response to @c VerifyGateway event.
    std::function<void(const std::shared_ptr<PostConnectVerifyGatewaySender>&)> m_gatewayVerifiedCallback;

    /// Mutex to synchronize access to @c PostConnectMessageRequest.
    std::mutex m_mutex;

    /// Flag that will be set when the abortOperation method is called.
    bool m_isStopping;

    /// The @c PostConnectMessageRequest used to send post connect messages.
    std::shared_ptr<avsCommon::avs::PostConnectMessageRequest> m_postConnectRequest;

    /// The @c WaitEvent to cancel retry waits.
    avsCommon::utils::WaitEvent m_wakeEvent;
};

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_POSTCONNECTVERIFYGATEWAYSENDER_H_
