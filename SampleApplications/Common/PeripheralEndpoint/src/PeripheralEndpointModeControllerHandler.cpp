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

#include <algorithm>

#include <acsdk/Sample/Console/ConsolePrinter.h>
#include <acsdk/Sample/PeripheralEndpoint/PeripheralEndpointModeControllerHandler.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
#define TAG "PeripheralEndpointModeControllerHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::modeController;

const std::string PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED = "Red";

const std::string PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN = "Green";

const std::string PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE = "Blue";

/**
 * Helper function to notify mode value change to the observers of @c ModeControllerObserverInterface.
 *
 * @param modeState The changed mode to be notified to the observer.
 * @param cause The change cause represented using @c AlexaStateChangeCauseType.
 * @param observers The list of observers to be notified.
 */
static void notifyObservers(
    const ModeControllerInterface::ModeState& modeState,
    avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause,
    const std::list<std::shared_ptr<ModeControllerObserverInterface>>& observers) {
    ACSDK_DEBUG5(LX(__func__));
    for (auto& observer : observers) {
        observer->onModeChanged(modeState, cause);
    }
}

std::shared_ptr<PeripheralEndpointModeControllerHandler> PeripheralEndpointModeControllerHandler::create(
    const std::string& endpointName,
    const std::string& instance) {
    auto modeControllerHandler = std::shared_ptr<PeripheralEndpointModeControllerHandler>(
        new PeripheralEndpointModeControllerHandler(endpointName, instance));
    return modeControllerHandler;
}

PeripheralEndpointModeControllerHandler::PeripheralEndpointModeControllerHandler(
    const std::string& endpointName,
    const std::string& instance) :
        m_endpointName{endpointName},
        m_instance{instance},
        m_modes{MODE_CONTROLLER_MODE_RED, MODE_CONTROLLER_MODE_GREEN, MODE_CONTROLLER_MODE_BLUE} {
    m_currentMode = m_modes[0];
}

ModeControllerInterface::ModeControllerConfiguration PeripheralEndpointModeControllerHandler::getConfiguration() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return {m_modes};
}

std::pair<AlexaResponseType, std::string> PeripheralEndpointModeControllerHandler::setMode(
    const std::string& mode,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<ModeControllerObserverInterface>> copyOfObservers;
    bool notifyObserver = false;

    std::unique_lock<std::mutex> lock(m_mutex);
    if (mode.empty() || std::find(m_modes.begin(), m_modes.end(), mode) == m_modes.end()) {
        ACSDK_ERROR(LX("setModeFailed").d("reason", "invalidMode").d("mode", mode));
        return std::make_pair(AlexaResponseType::VALUE_OUT_OF_RANGE, "invalidMode");
    }

    if (m_currentMode != mode) {
        ConsolePrinter::prettyPrint({"ENDPOINT: " + m_endpointName, "INSTANCE: " + m_instance, "MODE SET TO: " + mode});
        m_currentMode = mode;
        copyOfObservers = m_observers;
        notifyObserver = true;
    }

    lock.unlock();

    if (notifyObserver) {
        notifyObservers(
            ModeControllerInterface::ModeState{
                m_currentMode, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
            cause,
            copyOfObservers);
    }

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, std::string> PeripheralEndpointModeControllerHandler::adjustMode(
    int modeDelta,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<ModeControllerObserverInterface>> copyOfObservers;

    std::unique_lock<std::mutex> lock(m_mutex);
    auto itr = std::find(m_modes.begin(), m_modes.end(), m_currentMode);
    if (itr == m_modes.end()) {
        ACSDK_ERROR(LX("adjustModeFailed").d("reason", "currentModeInvalid"));
        return std::make_pair(AlexaResponseType::INTERNAL_ERROR, "currentModeInvalid");
    }
    itr += modeDelta;
    if (!(itr >= m_modes.begin() && itr < m_modes.end())) {
        ACSDK_ERROR(LX("adjustModeFailed").d("reason", "requestedModeInvalid").d("modeDelta", modeDelta));
        return std::make_pair(AlexaResponseType::INVALID_VALUE, "requestedModeInvalid");
    }

    m_currentMode = *itr;
    ConsolePrinter::prettyPrint(
        {"ENDPOINT: " + m_endpointName, "INSTANCE: " + m_instance, "ADJUSTED MODE TO: " + m_currentMode});
    copyOfObservers = m_observers;

    lock.unlock();
    notifyObservers(
        ModeControllerInterface::ModeState{
            m_currentMode, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
        cause,
        copyOfObservers);

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, avsCommon::utils::Optional<ModeControllerInterface::ModeState>>
PeripheralEndpointModeControllerHandler::getMode() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return std::make_pair(
        AlexaResponseType::SUCCESS,
        avsCommon::utils::Optional<ModeControllerInterface::ModeState>(ModeControllerInterface::ModeState{
            m_currentMode, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)}));
}

bool PeripheralEndpointModeControllerHandler::addObserver(std::shared_ptr<ModeControllerObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.push_back(observer);
    return true;
}

void PeripheralEndpointModeControllerHandler::removeObserver(
    const std::shared_ptr<ModeControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.remove(observer);
}

void PeripheralEndpointModeControllerHandler::setMode(const std::string& mode) {
    auto result = setMode(mode, AlexaStateChangeCauseType::APP_INTERACTION);
    if (AlexaResponseType::SUCCESS != result.first) {
        ACSDK_ERROR(LX("setModeFailed").d("AlexaResponseType", result.first).d("Description", result.second));
    } else {
        ACSDK_DEBUG5(LX(__func__).m("Success"));
    }
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
