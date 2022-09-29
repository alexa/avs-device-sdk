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

#include "acsdk/Sample/Endpoint/EndpointAlexaRecordControllerHandler.h"

#include <acsdk/Sample/Console/ConsolePrinter.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaRecordControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace alexaRecordControllerInterfaces;

/// The namespace for this capability agent.
static const char NAMESPACE[] = "Alexa.RecordController";

/// The supported version
static const char INTERFACE_VERSION[] = "3";

std::shared_ptr<EndpointAlexaRecordControllerHandler> EndpointAlexaRecordControllerHandler::create(
    std::string endpointName) {
    return std::shared_ptr<EndpointAlexaRecordControllerHandler>(
        new EndpointAlexaRecordControllerHandler(std::move(endpointName)));
}

EndpointAlexaRecordControllerHandler::EndpointAlexaRecordControllerHandler(std::string endpointName) :
        m_endpointName{std::move(endpointName)},
        m_isRecording{false} {
}

RecordControllerInterface::Response EndpointAlexaRecordControllerHandler::startRecording() {
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Start Recording"});
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_isRecording = true;
    }
    return RecordControllerInterface::Response{};
}

RecordControllerInterface::Response EndpointAlexaRecordControllerHandler::stopRecording() {
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Stop Recording"});
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_isRecording = false;
    }
    return RecordControllerInterface::Response{};
}

bool EndpointAlexaRecordControllerHandler::isRecording() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_isRecording;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
