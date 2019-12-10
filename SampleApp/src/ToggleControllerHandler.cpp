/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include "SampleApp/ToggleControllerHandler.h"

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
//#include <Settings/SettingStringConversion.h>

/// String to identify log entries originating from this file.
static const std::string TAG("ToggleControllerHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::toggleController;

std::shared_ptr<ToggleControllerHandler> ToggleControllerHandler::create() {
    auto toggleControllerHandler = std::shared_ptr<ToggleControllerHandler>(new ToggleControllerHandler());
    return toggleControllerHandler;
}

ToggleControllerHandler::ToggleControllerHandler() : m_currentToggleState{false} {
}

std::pair<AlexaResponseType, std::string> ToggleControllerHandler::setToggleState(
    bool state,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<ToggleControllerObserverInterface>> copyOfObservers;
    bool notifyObserver = false;

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_currentToggleState != state) {
            if (state) {
                ConsolePrinter::prettyPrint("TOGGLE CONTROLLER TURN-ON");
            } else {
                ConsolePrinter::prettyPrint("TOGGLE CONTROLLER TURN-OFF");
            }

            m_currentToggleState = state;
            copyOfObservers = m_observers;
            notifyObserver = true;
        }
    }

    if (notifyObserver) {
        notifyObservers(
            ToggleControllerInterface::ToggleState{
                m_currentToggleState, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
            cause,
            copyOfObservers);
    }

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, avsCommon::utils::Optional<ToggleControllerInterface::ToggleState>>
ToggleControllerHandler::getToggleState() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return std::make_pair(
        AlexaResponseType::SUCCESS,
        avsCommon::utils::Optional<ToggleControllerInterface::ToggleState>(ToggleControllerInterface::ToggleState{
            m_currentToggleState, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)}));
}

bool ToggleControllerHandler::addObserver(std::shared_ptr<ToggleControllerObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.push_back(observer);
    return true;
}

void ToggleControllerHandler::removeObserver(const std::shared_ptr<ToggleControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.remove(observer);
}

void ToggleControllerHandler::notifyObservers(
    const ToggleControllerInterface::ToggleState& toggleState,
    AlexaStateChangeCauseType cause,
    std::list<std::shared_ptr<ToggleControllerObserverInterface>> observers) {
    ACSDK_DEBUG5(LX(__func__));
    for (auto& observer : observers) {
        observer->onToggleStateChanged(toggleState, cause);
    }
}

void ToggleControllerHandler::setToggleState(bool toggleState) {
    auto result = setToggleState(toggleState, AlexaStateChangeCauseType::APP_INTERACTION);
    if (AlexaResponseType::SUCCESS != result.first) {
        ACSDK_ERROR(LX("setToggleStateFailed").d("AlexaResponseType", result.first).d("Description", result.second));
    } else {
        ACSDK_DEBUG5(LX(__func__).m("Success"));
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
