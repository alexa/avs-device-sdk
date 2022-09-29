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

#include <acsdk/Sample/Console/ConsolePrinter.h>
#include "acsdk/Sample/Endpoint/EndpointAlexaSeekControllerHandler.h"

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaSeekControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace acsdkAlexaSeekControllerInterfaces;

/// The namespace for this capability agent.
static const char* const NAMESPACE = "Alexa.SeekController";

/// The supported version
static const char* const INTERFACE_VERSION = "3";

/// The maximum timestamp of the current video or audio content in seconds
static const std::chrono::seconds MAX_MEDIA_POSITION_SECONDS{600};

std::shared_ptr<EndpointAlexaSeekControllerHandler> EndpointAlexaSeekControllerHandler::create(
    const std::string& endpointName) {
    auto seekControllerHandler =
        std::shared_ptr<EndpointAlexaSeekControllerHandler>(new EndpointAlexaSeekControllerHandler(endpointName));
    return seekControllerHandler;
}

EndpointAlexaSeekControllerHandler::EndpointAlexaSeekControllerHandler(const std::string& endpointName) :
        m_endpointName{endpointName},
        m_currentMediaPosition{0} {
}

/**
 * Helper function to log the seek information, for the purpose of verifying seek operations are
 * received by the handler.
 *
 * @param endpointName The name of the endpoint the capability is assoicated.
 * @param deltaPosition The delta value to seek the current audio or video content in milliseconds.
 */
static void logOperation(const std::string& endpointName, const std::chrono::milliseconds& deltaPosition) {
    static const std::string apiNameString("API Name: " + std::string(NAMESPACE));
    static const std::string apiVersionString("API Version: " + std::string(INTERFACE_VERSION));
    std::string endpointNameString("Endpoint: " + endpointName);
    std::string deltaPositionString("DeltaPositionInMilliseconds: " + std::to_string(deltaPosition.count()));

    ConsolePrinter::prettyPrint({apiNameString, apiVersionString, endpointNameString, deltaPositionString});
}

acsdkAlexaSeekControllerInterfaces::AlexaSeekControllerInterface::Response EndpointAlexaSeekControllerHandler::
    adjustSeekPosition(const std::chrono::milliseconds& deltaPosition) {
    logOperation(m_endpointName, deltaPosition);

    std::chrono::milliseconds newMediaPosition;
    {
        std::lock_guard<std::mutex> lock{m_mutex};

        newMediaPosition = m_currentMediaPosition;
        std::chrono::milliseconds maxMediaPosition = MAX_MEDIA_POSITION_SECONDS;
        newMediaPosition += deltaPosition;

        if (newMediaPosition < std::chrono::milliseconds::zero()) {
            newMediaPosition = std::chrono::milliseconds::zero();
        } else if (newMediaPosition > maxMediaPosition) {
            newMediaPosition = maxMediaPosition;
        }

        m_currentMediaPosition = newMediaPosition;
    }

    return AlexaSeekControllerInterface::Response{newMediaPosition};
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
