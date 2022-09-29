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
#include "acsdk/Sample/Endpoint/EndpointAlexaPlaybackControllerHandler.h"

#include <set>

#include <acsdk/Sample/Console/ConsolePrinter.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaPlaybackControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace acsdkAlexaPlaybackControllerInterfaces;

/// The namespace for this capability agent.
static const char* const NAMESPACE = "Alexa.PlaybackController";

/// The supported version
static const char* const INTERFACE_VERSION = "3";

void EndpointAlexaPlaybackControllerHandler::logOperation(
    const std::string& endpointName,
    const std::string& playbackOperation) {
    static const std::string apiNameString("API Name: " + std::string(NAMESPACE));
    static const std::string apiVersionString("API Version: " + std::string(INTERFACE_VERSION));
    std::string endpointNameString("Endpoint: " + endpointName);
    std::string playbackOperationString("Playback Operation: " + playbackOperation);

    ConsolePrinter::prettyPrint({apiNameString, apiVersionString, endpointNameString, playbackOperationString});
}

std::shared_ptr<EndpointAlexaPlaybackControllerHandler> EndpointAlexaPlaybackControllerHandler::create(
    const std::string& endpointName,
    std::shared_ptr<EndpointFocusAdapter> focusAdapter) {
    if (!focusAdapter) {
        ACSDK_WARN(LX(__func__).m("NULL Focus Adapter"));
    }
    auto playbackControllerHandler = std::shared_ptr<EndpointAlexaPlaybackControllerHandler>(
        new EndpointAlexaPlaybackControllerHandler(endpointName, std::move(focusAdapter)));
    return playbackControllerHandler;
}

EndpointAlexaPlaybackControllerHandler::EndpointAlexaPlaybackControllerHandler(
    const std::string& endpointName,
    std::shared_ptr<EndpointFocusAdapter> focusAdapter) :
        m_endpointName{endpointName},
        m_currentPlaybackState{PlaybackState::STOPPED},
        m_focusAdapter{std::move(focusAdapter)} {
    m_notifier = std::make_shared<AlexaPlaybackControllerNotifier>();
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::play() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_currentPlaybackState = PlaybackState::PLAYING;
    }

    // Play the current media
    logOperation(m_endpointName, "PLAY");

    notifyObservers(m_currentPlaybackState);

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::pause() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_currentPlaybackState = PlaybackState::PAUSED;
    }

    // Pause the current media
    logOperation(m_endpointName, "PAUSE");

    notifyObservers(m_currentPlaybackState);

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_currentPlaybackState = PlaybackState::STOPPED;
    }

    // Stop the current media
    logOperation(m_endpointName, "STOP");

    notifyObservers(m_currentPlaybackState);

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::startOver() {
    // Restart the current media
    logOperation(m_endpointName, "START_OVER");

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::previous() {
    // Go to the previous media
    logOperation(m_endpointName, "PREVIOUS");

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::next() {
    // Go to the next media
    logOperation(m_endpointName, "NEXT");

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::rewind() {
    // Rewind the current media
    logOperation(m_endpointName, "REWIND");

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

AlexaPlaybackControllerInterface::Response EndpointAlexaPlaybackControllerHandler::fastForward() {
    // Fast forward the current media
    logOperation(m_endpointName, "FAST_FORWARD");

    return AlexaPlaybackControllerInterface::Response{AlexaPlaybackControllerInterface::Response::Type::SUCCESS, ""};
}

PlaybackState EndpointAlexaPlaybackControllerHandler::getPlaybackState() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_currentPlaybackState;
}

bool EndpointAlexaPlaybackControllerHandler::addObserver(
    const std::weak_ptr<AlexaPlaybackControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_notifier->addWeakPtrObserver(observer);
    return true;
}

void EndpointAlexaPlaybackControllerHandler::removeObserver(
    const std::weak_ptr<AlexaPlaybackControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_notifier->removeWeakPtrObserver(observer);
}

void EndpointAlexaPlaybackControllerHandler::notifyObservers(PlaybackState playbackState) {
    ACSDK_DEBUG5(LX(__func__));
    m_notifier->notifyObservers(
        [playbackState](const std::shared_ptr<AlexaPlaybackControllerObserverInterface>& observer) {
            if (observer) {
                observer->onPlaybackStateChanged(playbackState);
            }
        });
}

std::set<PlaybackOperation> EndpointAlexaPlaybackControllerHandler::getSupportedOperations() {
    static const std::set<PlaybackOperation> playbackOperations = {PlaybackOperation::PLAY,
                                                                   PlaybackOperation::PAUSE,
                                                                   PlaybackOperation::STOP,
                                                                   PlaybackOperation::START_OVER,
                                                                   PlaybackOperation::PREVIOUS,
                                                                   PlaybackOperation::NEXT,
                                                                   PlaybackOperation::REWIND,
                                                                   PlaybackOperation::FAST_FORWARD};
    return playbackOperations;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
