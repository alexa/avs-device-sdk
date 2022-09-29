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
#include "acsdk/Sample/Endpoint/EndpointAlexaVideoRecorderHandler.h"

#include <acsdk/Sample/Console/ConsolePrinter.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaVideoRecorderHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace avsCommon::utils;
using namespace acsdkAlexaVideoRecorderInterfaces;

std::unique_ptr<EndpointAlexaVideoRecorderHandler> EndpointAlexaVideoRecorderHandler::create(
    const std::string endpointName) {
    auto videoRecorderHandler = std::unique_ptr<EndpointAlexaVideoRecorderHandler>(
        new EndpointAlexaVideoRecorderHandler(std::move(endpointName)));
    return videoRecorderHandler;
}

EndpointAlexaVideoRecorderHandler::EndpointAlexaVideoRecorderHandler(const std::string& endpointName) :
        m_endpointName{endpointName} {
}

VideoRecorderInterface::Response EndpointAlexaVideoRecorderHandler::searchAndRecord(
    std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest>) {
    std::unique_lock<std::mutex> lock(m_mutex);

    ConsolePrinter::prettyPrint({"ENDPOINT: " + m_endpointName, "Search and Record"});
    lock.unlock();

    VideoRecorderInterface::Response videoRecorderResponse;
    videoRecorderResponse.type = VideoRecorderInterface::Response::Type::SUCCESS;
    videoRecorderResponse.message = "SCHEDULED";

    return videoRecorderResponse;
}

VideoRecorderInterface::Response EndpointAlexaVideoRecorderHandler::cancelRecording(
    std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest>) {
    std::unique_lock<std::mutex> lock(m_mutex);

    ConsolePrinter::prettyPrint({"ENDPOINT: " + m_endpointName, "Cancel Recording Results"});
    lock.unlock();

    VideoRecorderInterface::Response videoRecorderResponse;
    return videoRecorderResponse;
}

VideoRecorderInterface::Response EndpointAlexaVideoRecorderHandler::deleteRecording(
    std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest>) {
    std::unique_lock<std::mutex> lock(m_mutex);

    ConsolePrinter::prettyPrint({"ENDPOINT: " + m_endpointName, "Delete Recording Results"});
    lock.unlock();

    VideoRecorderInterface::Response videoRecorderResponse;
    return videoRecorderResponse;
}

bool EndpointAlexaVideoRecorderHandler::isExtendedRecordingGUIShown() {
    // must come from the physical or virtual endpoint device
    bool retVal = true;
    return retVal;
}

int EndpointAlexaVideoRecorderHandler::getStorageUsedPercentage() {
    // must come from the physical or virtual endpoint device
    int retVal = 100;
    return retVal;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
