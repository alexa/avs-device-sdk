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

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "SampleApp/PeripheralEndpoint/PeripheralEndpointPowerControllerHandler.h"
#include "SampleApp/ConsolePrinter.h"

/// String to identify log entries originating from this file.
static const std::string TAG("PeripheralEndpointPowerControllerHandler");

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
using namespace avsCommon::sdkInterfaces::powerController;

/**
 * Helper function to notify power state change to the observers of @c PowerControllerObserverInterface.
 *
 * @param powerState The changed power state to be notified to the observer.
 * @param cause The change cause represented using @c AlexaStateChangeCauseType.
 * @param observers The list of observer to be notified.
 */
static void notifyObservers(
    const avsCommon::sdkInterfaces::powerController::PowerControllerInterface::PowerState& powerState,
    avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause,
    const std::list<std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface>>&
        observers) {
    ACSDK_DEBUG5(LX(__func__));
    for (auto& observer : observers) {
        observer->onPowerStateChanged(powerState, cause);
    }
}

std::shared_ptr<PeripheralEndpointPowerControllerHandler> PeripheralEndpointPowerControllerHandler::create(
    const std::string& endpointName) {
    auto powerControllerHandler = std::shared_ptr<PeripheralEndpointPowerControllerHandler>(
        new PeripheralEndpointPowerControllerHandler(endpointName));
    return powerControllerHandler;
}

PeripheralEndpointPowerControllerHandler::PeripheralEndpointPowerControllerHandler(const std::string& endpointName) :
        m_endpointName{endpointName},
        m_currentPowerState{false} {
}

std::pair<AlexaResponseType, std::string> PeripheralEndpointPowerControllerHandler::setPowerState(
    bool state,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<PowerControllerObserverInterface>> copyOfObservers;
    bool notifyObserver = false;

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_currentPowerState != state) {
        std::string stateStr = state ? "ON" : "OFF";
        ConsolePrinter::prettyPrint({"ENDPOINT: " + m_endpointName, "POWER STATE: " + stateStr});

        m_currentPowerState = state;
        copyOfObservers = m_observers;
        notifyObserver = true;
    }

    m_currentPowerState = state;
    copyOfObservers = m_observers;
    notifyObserver = true;

    lock.unlock();

    if (notifyObserver) {
        notifyObservers(
            PowerControllerInterface::PowerState{
                m_currentPowerState, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
            cause,
            copyOfObservers);
    }

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, avsCommon::utils::Optional<PowerControllerInterface::PowerState>>
PeripheralEndpointPowerControllerHandler::getPowerState() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return std::make_pair(
        AlexaResponseType::SUCCESS,
        avsCommon::utils::Optional<PowerControllerInterface::PowerState>(PowerControllerInterface::PowerState{
            m_currentPowerState, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)}));
}

bool PeripheralEndpointPowerControllerHandler::addObserver(std::shared_ptr<PowerControllerObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.push_back(observer);
    return true;
}

void PeripheralEndpointPowerControllerHandler::removeObserver(
    const std::shared_ptr<PowerControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.remove(observer);
}

void PeripheralEndpointPowerControllerHandler::setPowerState(bool powerState) {
    auto result = setPowerState(powerState, AlexaStateChangeCauseType::APP_INTERACTION);
    if (AlexaResponseType::SUCCESS != result.first) {
        ACSDK_ERROR(LX("setPowerStateFailed").d("AlexaResponseType", result.first).d("Description", result.second));
    } else {
        ACSDK_DEBUG5(LX(__func__).m("Success"));
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
