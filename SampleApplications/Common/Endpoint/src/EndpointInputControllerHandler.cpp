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

#include "acsdk/Sample/Endpoint/EndpointInputControllerHandler.h"

#include <acsdk/Sample/Console/ConsolePrinter.h>

/// Literal to identify log entries originating from this file.
#define TAG "EndpointInputControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace alexaInputControllerInterfaces;

static const Input DEFAULT_INPUT = Input::HDMI_1;

/// Inputs available on Device
static const InputControllerInterface::SupportedInputs INPUT_AVAILABLE{{Input::HDMI_1, {"TV", "Television"}},
                                                                       {Input::DVD, {"DVD"}},
                                                                       {Input::CD, {"MUSIC"}}};

std::shared_ptr<EndpointInputControllerHandler> EndpointInputControllerHandler::create(
    const std::string& endpointName) {
    return std::shared_ptr<EndpointInputControllerHandler>(new EndpointInputControllerHandler(endpointName));
}

EndpointInputControllerHandler::EndpointInputControllerHandler(const std::string& endpointName) :
        m_endpointName{endpointName},
        m_currentInput{DEFAULT_INPUT},
        m_supportedInputs{INPUT_AVAILABLE} {
}

InputControllerInterface::Response EndpointInputControllerHandler::setInput(Input input) {
    std::string prefix = m_endpointName.empty() ? "DEFAULT" : m_endpointName;
    ConsolePrinter::prettyPrint({" ENDPOINT: " + prefix + " Input: " + inputToString(input)});
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentInput = input;
    }
    m_notifier.notifyObservers(
        [&](const std::shared_ptr<InputControllerObserverInterface>& observer) { observer->onInputChanged(input); });
    return InputControllerInterface::Response();
}

InputControllerInterface::SupportedInputs EndpointInputControllerHandler::getSupportedInputs() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_supportedInputs;
}

Input EndpointInputControllerHandler::getInput() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_currentInput;
}

bool EndpointInputControllerHandler::addObserver(const std::weak_ptr<InputControllerObserverInterface> observer) {
    m_notifier.addWeakPtrObserver(observer);
    return true;
}

void EndpointInputControllerHandler::removeObserver(const std::weak_ptr<InputControllerObserverInterface> observer) {
    m_notifier.removeWeakPtrObserver(observer);
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
