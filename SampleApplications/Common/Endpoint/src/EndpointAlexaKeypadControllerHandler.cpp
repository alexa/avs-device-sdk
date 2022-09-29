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
#include "acsdk/Sample/Endpoint/EndpointAlexaKeypadControllerHandler.h"

#include <acsdk/Sample/Console/ConsolePrinter.h>

/// String to identify log entries originating from this file.
#define TAG "AlexaKeypadControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace acsdkAlexaKeypadControllerInterfaces;

/// The namespace for this capability agent.
static const char* const NAMESPACE = "Alexa.KeypadController";

/// The supported version
static const char* const INTERFACE_VERSION = "3";

std::shared_ptr<EndpointAlexaKeypadControllerHandler> EndpointAlexaKeypadControllerHandler::create(
    const std::string& endpointName) {
    auto keypadControllerHandler =
        std::shared_ptr<EndpointAlexaKeypadControllerHandler>(new EndpointAlexaKeypadControllerHandler(endpointName));
    return keypadControllerHandler;
}

EndpointAlexaKeypadControllerHandler::EndpointAlexaKeypadControllerHandler(const std::string& endpointName) :
        m_endpointName{endpointName} {
}

AlexaKeypadControllerInterface::Response EndpointAlexaKeypadControllerHandler::handleKeystroke(Keystroke keystroke) {
    std::unique_lock<std::mutex> lock(m_mutex);

    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Keystroke: " + keystrokeToString(keystroke)});

    lock.unlock();
    return AlexaKeypadControllerInterface::Response();
}

std::set<Keystroke> EndpointAlexaKeypadControllerHandler::getSupportedKeys() {
    const static std::set<Keystroke> keysSet = {Keystroke::INFO,
                                                Keystroke::MORE,
                                                Keystroke::SELECT,
                                                Keystroke::UP,
                                                Keystroke::DOWN,
                                                Keystroke::LEFT,
                                                Keystroke::RIGHT,
                                                Keystroke::BACK,
                                                Keystroke::PAGE_UP,
                                                Keystroke::PAGE_DOWN,
                                                Keystroke::PAGE_LEFT,
                                                Keystroke::PAGE_RIGHT};
    return keysSet;
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
