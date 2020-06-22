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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_POSTCONNECTCAPABILITIESPUBLISHER_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_POSTCONNECTCAPABILITIESPUBLISHER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <AVSCommon/AVS/WaitableMessageRequest.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <CapabilitiesDelegate/DiscoveryEventSenderInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * This class is responsible publishing @c Discovery.AddOrUpdateReport and @c Discovery.DeleteReport events in the
 * post connecting state.
 *
 * @Note: A new instance of the PostConnectCapabilitiesPublisher will be created as part of the post connect sequence.
 */
class PostConnectCapabilitiesPublisher
        : public avsCommon::sdkInterfaces::PostConnectOperationInterface
        , public std::enable_shared_from_this<PostConnectCapabilitiesPublisher> {
public:
    /**
     * Creates a new instance of the @c PostConnectCapabilitiesPublisher.
     *
     * @param discoveryEventSender The @c DiscoveryEventSender to send discovery events.
     * @return a new instance of the @c PostConnectCapabilitiesPublisher.
     */
    static std::shared_ptr<PostConnectCapabilitiesPublisher> create(
        const std::shared_ptr<DiscoveryEventSenderInterface>& discoveryEventSender);

    /**
     * Destructor.
     */
    ~PostConnectCapabilitiesPublisher();

    /// PostConnectOperationInterface Methods.
    /// @{
    unsigned int getOperationPriority() override;
    bool performOperation(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) override;
    void abortOperation() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param discoveryEventSender The @c DiscoveryEventSenderInterface to send discovery events.
     */
    PostConnectCapabilitiesPublisher(std::shared_ptr<DiscoveryEventSenderInterface> discoveryEventSender);

    /// Mutex to synchronize access to @c m_isPerformOperationInvoked.
    std::mutex m_mutex;

    /// Flag to guard against repeated calls to performOperation() method.
    bool m_isPerformOperationInvoked;

    /// @c DiscoveryEventSenderInterface for sending Discovery events.
    std::shared_ptr<DiscoveryEventSenderInterface> m_discoveryEventSender;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_POSTCONNECTCAPABILITIESPUBLISHER_H_
