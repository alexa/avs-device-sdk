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

#include "acsdk/Sample/Endpoint/EndpointAlexaRemoteVideoPlayerHandler.h"
#include <acsdk/Sample/Console/ConsolePrinter.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointRemoteVideoPlayerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace alexaRemoteVideoPlayerInterfaces;
using namespace acsdkAlexaVideoCommon;
using namespace presentationOrchestratorInterfaces;
using namespace avsCommon::sdkInterfaces;

/// The supported version
static const char* const INTERFACE_VERSION = "3";

/// Interface name for Alexa.RemoteVideoPlayer requests.
static const char ALEXA_REMOTE_VIDEO_PLAYER_INTERFACE_NAME[] = "Alexa.RemoteVideoPlayer";

std::shared_ptr<EndpointAlexaRemoteVideoPlayerHandler> EndpointAlexaRemoteVideoPlayerHandler::create(
    std::string endpointName,
    std::shared_ptr<EndpointFocusAdapter> focusAdapter) {
    if (!focusAdapter) {
        ACSDK_WARN(LX(__func__).m("NULL Focus Adapter"));
    }
    auto remoteVideoPlayerHandler = std::shared_ptr<EndpointAlexaRemoteVideoPlayerHandler>(
        new EndpointAlexaRemoteVideoPlayerHandler(std::move(endpointName), std::move(focusAdapter)));
    return remoteVideoPlayerHandler;
}

EndpointAlexaRemoteVideoPlayerHandler::EndpointAlexaRemoteVideoPlayerHandler(
    std::string endpointName,
    std::shared_ptr<EndpointFocusAdapter> focusAdapter) :
        m_endpointName{std::move(endpointName)},
        m_focusAdapter{std::move(focusAdapter)} {
}

RemoteVideoPlayerInterface::Response EndpointAlexaRemoteVideoPlayerHandler::playVideo(
    std::unique_ptr<RemoteVideoPlayerRequest> request) {
    std::string queryPayload = request->searchTextTranscribed.valueOr("[Unable to transcribe Search Query]");
    ConsolePrinter::prettyPrint({"API Name: " + std::string(ALEXA_REMOTE_VIDEO_PLAYER_INTERFACE_NAME),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Play Video Query:" + queryPayload});
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_focusAdapter) {
        m_focusAdapter->acquireFocus(ALEXA_REMOTE_VIDEO_PLAYER_INTERFACE_NAME, [this]() { onFocusAcquired(); });
    }
    lock.unlock();
    return RemoteVideoPlayerInterface::Response();
}

RemoteVideoPlayerInterface::Response EndpointAlexaRemoteVideoPlayerHandler::displaySearchResults(
    std::unique_ptr<RemoteVideoPlayerRequest> request) {
    std::string queryPayload = request->searchTextTranscribed.valueOr("[Unable to transcribe Search Query]");
    ConsolePrinter::prettyPrint({"API Name: " + std::string(ALEXA_REMOTE_VIDEO_PLAYER_INTERFACE_NAME),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Display Search Results Query:" + queryPayload});
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_focusAdapter) {
        m_focusAdapter->acquireFocus(ALEXA_REMOTE_VIDEO_PLAYER_INTERFACE_NAME, [this]() { onFocusAcquired(); });
    }
    lock.unlock();
    return RemoteVideoPlayerInterface::Response();
}

Configuration EndpointAlexaRemoteVideoPlayerHandler::getConfiguration() {
    const static std::set<Configuration::SupportedOperations> directives{
        Configuration::SupportedOperations::PLAY_VIDEO, Configuration::SupportedOperations::DISPLAY_SEARCH_RESULTS};

    const static std::set<acsdkAlexaVideoCommon::VideoEntity::EntityType> entityTypes{
        VideoEntity::EntityType::MEDIA_TYPE,
        VideoEntity::EntityType::ACTOR,
        VideoEntity::EntityType::CHARACTER,
        VideoEntity::EntityType::EVENT,
        VideoEntity::EntityType::FRANCHISE,
        VideoEntity::EntityType::GENRE,
        VideoEntity::EntityType::LEAGUE,
        VideoEntity::EntityType::SEASON,
        VideoEntity::EntityType::EPISODE,
        VideoEntity::EntityType::SPORT,
        VideoEntity::EntityType::SPORTS_TEAM,
        VideoEntity::EntityType::VIDEO};

    const static std::list<Configuration::Catalog> catalog{Configuration::Catalog()};

    return Configuration(directives, entityTypes, catalog);
}

void EndpointAlexaRemoteVideoPlayerHandler::onFocusAcquired() {
    ACSDK_DEBUG9(LX(__func__));
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
