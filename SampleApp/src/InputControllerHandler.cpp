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

#include "SampleApp/ConsolePrinter.h"
#include "SampleApp/InputControllerHandler.h"

/// String to identify log entries originating from this file.
static const std::string TAG("InputControllerHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApp {

const static acsdkInputControllerInterfaces::InputControllerHandlerInterface::InputFriendlyNameType inputs{
    {"HDMI1", {"TV", "Television"}},
    {"PHONO", {"Turntable"}},
    {"AUX1", {"Cassette"}}};

std::shared_ptr<InputControllerHandler> InputControllerHandler::create() {
    auto inputControllerHandler = std::shared_ptr<InputControllerHandler>(new InputControllerHandler());
    return inputControllerHandler;
}

acsdkInputControllerInterfaces::InputControllerHandlerInterface::InputConfigurations InputControllerHandler::
    getConfiguration() {
    return {inputs};
}

bool InputControllerHandler::onInputChange(const std::string& input) {
    ConsolePrinter::prettyPrint({"InputController Input: " + input});
    return true;
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
