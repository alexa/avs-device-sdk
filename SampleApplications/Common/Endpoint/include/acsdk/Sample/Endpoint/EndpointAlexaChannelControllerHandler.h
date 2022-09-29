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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXACHANNELCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXACHANNELCONTROLLERHANDLER_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <acsdk/AlexaChannelControllerInterfaces/ChannelControllerInterface.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaChannelControllerInterface.
 */

class EndpointAlexaChannelControllerHandler : public alexaChannelControllerInterfaces::ChannelControllerInterface {
public:
    /**
     * Create a EndpointAlexaChannelControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new EndpointAlexaChannelControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaChannelControllerHandler> create(std::string endpointName);

    /// @name AlexaChannelControllerInterface methods
    /// @{
    alexaChannelControllerInterfaces::ChannelControllerInterface::Response change(
        std::unique_ptr<alexaChannelControllerTypes::Channel> channel) override;
    alexaChannelControllerInterfaces::ChannelControllerInterface::Response incrementChannel() override;
    alexaChannelControllerInterfaces::ChannelControllerInterface::Response decrementChannel() override;
    std::unique_ptr<alexaChannelControllerTypes::Channel> getCurrentChannel() override;
    bool addObserver(
        std::weak_ptr<alexaChannelControllerInterfaces::ChannelControllerObserverInterface> observer) override;
    void removeObserver(
        std::weak_ptr<alexaChannelControllerInterfaces::ChannelControllerObserverInterface> observer) override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaChannelControllerHandler(std::string endpointName);

    /**
     * Helper function to notify channel change to the observers of @c ChannelControllerInterface.
     *
     * @param channel The changed channel to be notified to the observer.
     */
    void notifyObservers(std::unique_ptr<alexaChannelControllerTypes::Channel> channel);

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// Current channel at the endpoint.
    std::unique_ptr<alexaChannelControllerTypes::Channel> m_currentChannel;

    /// The notifier of @c ChannelControllerInterface observers.
    notifier::Notifier<alexaChannelControllerInterfaces::ChannelControllerObserverInterface> m_notifier;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXACHANNELCONTROLLERHANDLER_H_
