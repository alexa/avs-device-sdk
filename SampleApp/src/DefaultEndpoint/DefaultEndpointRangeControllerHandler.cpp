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
#include "SampleApp/DefaultEndpoint/DefaultEndpointRangeControllerHandler.h"

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// The range controller maximum range.
static const double MAXIMUM_RANGE = 5;

/// The range controller minimum range.
static const double MINIMUM_RANGE = 1;

/// The range controller precision.
static const double RANGE_PRECISION = 1;

/// String to identify log entries originating from this file.
static const std::string TAG("DefaultEndpointRangeControllerHandler");

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
using namespace avsCommon::sdkInterfaces::rangeController;

/**
 * Helper function to notify range value change to the observers of @c RangeControllerObserverInterface.
 *
 * @param rangeValue The changed range value to be notified to the observer.
 * @param cause The change cause represented using @c AlexaStateChangeCauseType.
 * @param observers The list of observer to be notified.
 */
static void notifyObservers(
    const avsCommon::sdkInterfaces::rangeController::RangeControllerInterface::RangeState& rangeState,
    avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause,
    const std::list<std::shared_ptr<avsCommon::sdkInterfaces::rangeController::RangeControllerObserverInterface>>&
        observers) {
    ACSDK_DEBUG5(LX(__func__));
    for (auto& observer : observers) {
        observer->onRangeChanged(rangeState, cause);
    }
}

std::shared_ptr<DefaultEndpointRangeControllerHandler> DefaultEndpointRangeControllerHandler::create(
    const std::string& instance) {
    auto rangeControllerHandler =
        std::shared_ptr<DefaultEndpointRangeControllerHandler>(new DefaultEndpointRangeControllerHandler(instance));
    return rangeControllerHandler;
}

DefaultEndpointRangeControllerHandler::DefaultEndpointRangeControllerHandler(const std::string& instance) :
        m_instance{instance},
        m_currentRangeValue{MINIMUM_RANGE},
        m_maximumRangeValue{MAXIMUM_RANGE},
        m_minmumRangeValue{MINIMUM_RANGE},
        m_precision{RANGE_PRECISION} {
}

RangeControllerInterface::RangeControllerConfiguration DefaultEndpointRangeControllerHandler::getConfiguration() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return {m_minmumRangeValue, m_maximumRangeValue, m_precision};
}

std::pair<AlexaResponseType, std::string> DefaultEndpointRangeControllerHandler::setRangeValue(
    double value,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<RangeControllerObserverInterface>> copyOfObservers;
    bool notifyObserver = false;

    std::unique_lock<std::mutex> lock(m_mutex);
    if (value < m_minmumRangeValue || value > m_maximumRangeValue) {
        ACSDK_ERROR(LX("setRangeValueFailed").d("reason", "valueOutOfSupportedRange").d("value", value));
        return std::make_pair(AlexaResponseType::VALUE_OUT_OF_RANGE, "valueOutOfSupportedRange");
    }

    if (m_currentRangeValue != value) {
        ConsolePrinter::prettyPrint(
            {"ENDPOINT: Default Endpoint", "INSTANCE: " + m_instance, "RANGE SET TO: " + std::to_string(value)});
        m_currentRangeValue = value;
        copyOfObservers = m_observers;
        notifyObserver = true;
    }

    lock.unlock();

    if (notifyObserver) {
        notifyObservers(
            RangeControllerInterface::RangeState{
                m_currentRangeValue, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
            cause,
            copyOfObservers);
    }

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, std::string> DefaultEndpointRangeControllerHandler::adjustRangeValue(
    double deltaValue,
    AlexaStateChangeCauseType cause) {
    std::list<std::shared_ptr<RangeControllerObserverInterface>> copyOfObservers;
    bool notifyObserver = false;

    std::unique_lock<std::mutex> lock(m_mutex);
    auto newvalue = m_currentRangeValue + deltaValue;
    if (newvalue < m_minmumRangeValue || newvalue > m_maximumRangeValue) {
        ACSDK_ERROR(LX("adjustRangeValueFailed").d("reason", "requestedRangeValueInvalid").d("deltaValue", deltaValue));
        return std::make_pair(AlexaResponseType::VALUE_OUT_OF_RANGE, "ValueOutOfRange");
    }

    m_currentRangeValue += deltaValue;
    ConsolePrinter::prettyPrint({"ENDPOINT: Default Endpoint",
                                 "INSTANCE: " + m_instance,
                                 "ADJUSTED RANGE TO : " + std::to_string(m_currentRangeValue)});
    copyOfObservers = m_observers;
    notifyObserver = true;

    lock.unlock();

    if (notifyObserver) {
        notifyObservers(
            RangeControllerInterface::RangeState{
                m_currentRangeValue, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)},
            cause,
            copyOfObservers);
    }

    return std::make_pair(AlexaResponseType::SUCCESS, "");
}

std::pair<AlexaResponseType, avsCommon::utils::Optional<RangeControllerInterface::RangeState>>
DefaultEndpointRangeControllerHandler::getRangeState() {
    std::lock_guard<std::mutex> lock{m_mutex};
    return std::make_pair(
        AlexaResponseType::SUCCESS,
        avsCommon::utils::Optional<RangeControllerInterface::RangeState>(RangeControllerInterface::RangeState{
            m_currentRangeValue, avsCommon::utils::timing::TimePoint::now(), std::chrono::milliseconds(0)}));
}

bool DefaultEndpointRangeControllerHandler::addObserver(std::shared_ptr<RangeControllerObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.push_back(observer);
    return true;
}

void DefaultEndpointRangeControllerHandler::removeObserver(
    const std::shared_ptr<RangeControllerObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.remove(observer);
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
