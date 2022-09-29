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

#include "acsdk/Sample/Endpoint/EndpointAlexaChannelControllerHandler.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <acsdk/Sample/Console/ConsolePrinter.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaChannelControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace alexaChannelControllerInterfaces;
using namespace alexaChannelControllerTypes;

/// The namespace for this capability agent.
static const char NAMESPACE[] = "Alexa.ChannelController";

/// The supported version
static const char INTERFACE_VERSION[] = "3";

/// The starting channel number
static const char INITIAL_CHANNEL_NUMBER[] = "1";

std::shared_ptr<EndpointAlexaChannelControllerHandler> EndpointAlexaChannelControllerHandler::create(
    std::string endpointName) {
    auto channelControllerHandler = std::shared_ptr<EndpointAlexaChannelControllerHandler>(
        new EndpointAlexaChannelControllerHandler(std::move(endpointName)));
    return channelControllerHandler;
}

/**
 * A helper function to generate string out of Channel object.
 * @param channel Channel related values such as name, callsign, affiliate callsign and uri, name and image.
 * @return A string object that contains Channel content
 */
static std::string generateChannelPrint(const Channel& channel) {
    std::string channelPayload = " Channel {  Number: " + channel.getNumber() + ", CallSign: " + channel.getCallSign() +
                                 ", Affiliate CallSign:" + channel.getAffiliateCallSign() +
                                 ", URI: " + channel.getURI() + +" Channel Metadata { Name: " + channel.getName() +
                                 ", Image: " + channel.getImageURL() + " } " + " } ";
    return "{" + channelPayload + " } ";
}

EndpointAlexaChannelControllerHandler::EndpointAlexaChannelControllerHandler(std::string endpointName) :
        m_endpointName{std::move(endpointName)},
        m_currentChannel{Channel::Create(INITIAL_CHANNEL_NUMBER, "", "", "", "", "")} {
}

ChannelControllerInterface::Response EndpointAlexaChannelControllerHandler::change(std::unique_ptr<Channel> channel) {
    if (channel == nullptr) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalidChannel"));
        return ChannelControllerInterface::Response{ChannelControllerInterface::Response::Type::FAILED_INTERNAL_ERROR,
                                                    "Channel is nullptr"};
    }
    std::string channelPayload = generateChannelPrint(*channel);
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Change Channel",
                                 +"Channel Payload: " + channelPayload});
    std::unique_lock<std::mutex> lock{m_mutex};
    m_currentChannel = std::move(channel);
    std::unique_ptr<Channel> copyOfChannel = Channel::Create(
        m_currentChannel->getNumber(),
        m_currentChannel->getCallSign(),
        m_currentChannel->getAffiliateCallSign(),
        m_currentChannel->getURI(),
        m_currentChannel->getName(),
        m_currentChannel->getImageURL());
    lock.unlock();
    notifyObservers(std::move(copyOfChannel));

    return ChannelControllerInterface::Response{};
}

ChannelControllerInterface::Response EndpointAlexaChannelControllerHandler::incrementChannel() {
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Increment Channel"});
    std::unique_lock<std::mutex> lock{m_mutex};
    std::unique_ptr<Channel> copyOfChannel = Channel::Create(
        m_currentChannel->getNumber(),
        m_currentChannel->getCallSign(),
        m_currentChannel->getAffiliateCallSign(),
        m_currentChannel->getURI(),
        m_currentChannel->getName(),
        m_currentChannel->getImageURL());
    lock.unlock();

    notifyObservers(std::move(copyOfChannel));

    return ChannelControllerInterface::Response{};
}

ChannelControllerInterface::Response EndpointAlexaChannelControllerHandler::decrementChannel() {
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Decrement Channel"});
    std::unique_lock<std::mutex> lock{m_mutex};
    std::unique_ptr<Channel> copyOfChannel = Channel::Create(
        m_currentChannel->getNumber(),
        m_currentChannel->getCallSign(),
        m_currentChannel->getAffiliateCallSign(),
        m_currentChannel->getURI(),
        m_currentChannel->getName(),
        m_currentChannel->getImageURL());
    lock.unlock();

    notifyObservers(std::move(copyOfChannel));

    return ChannelControllerInterface::Response{};
}

std::unique_ptr<Channel> EndpointAlexaChannelControllerHandler::getCurrentChannel() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return Channel::Create(
        m_currentChannel->getNumber(),
        m_currentChannel->getCallSign(),
        m_currentChannel->getAffiliateCallSign(),
        m_currentChannel->getURI(),
        m_currentChannel->getName(),
        m_currentChannel->getImageURL());
}

bool EndpointAlexaChannelControllerHandler::addObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) {
    m_notifier.addWeakPtrObserver(observer);
    return true;
}

void EndpointAlexaChannelControllerHandler::removeObserver(std::weak_ptr<ChannelControllerObserverInterface> observer) {
    m_notifier.removeWeakPtrObserver(observer);
}

void EndpointAlexaChannelControllerHandler::notifyObservers(std::unique_ptr<Channel> channelState) {
    ACSDK_DEBUG9(LX(__func__));
    if (channelState == nullptr) {
        ACSDK_ERROR(LX(__func__).d("reason:", "channelState is nullptr"));
        return;
    }
    std::shared_ptr<Channel> channel = std::move(channelState);
    m_notifier.notifyObservers([channel](const std::shared_ptr<ChannelControllerObserverInterface>& observer) {
        std::unique_ptr<Channel> state = Channel::Create(
            channel->getNumber(),
            channel->getCallSign(),
            channel->getAffiliateCallSign(),
            channel->getURI(),
            channel->getName(),
            channel->getImageURL());
        observer->onChannelChanged(std::move(state));
    });
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
