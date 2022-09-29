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

#include "acsdk/Sample/Endpoint/EndpointAlexaLauncherHandler.h"

#include <AVSCommon/Utils/Logger/Logger.h>
#include <acsdk/Sample/Console/ConsolePrinter.h>

/// String to identify log entries originating from this file.
#define TAG "EndpointAlexaLauncherHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

using namespace acsdkAlexaLauncherInterfaces;

/// The namespace for this capability agent.
static const char* const NAMESPACE = "Alexa.Launcher";

/// The supported version
static const char* const INTERFACE_VERSION = "3";

std::shared_ptr<EndpointAlexaLauncherHandler> EndpointAlexaLauncherHandler::create(const std::string& endpointName) {
    auto launcherHandler =
        std::shared_ptr<EndpointAlexaLauncherHandler>(new EndpointAlexaLauncherHandler(endpointName));
    return launcherHandler;
}

EndpointAlexaLauncherHandler::EndpointAlexaLauncherHandler(const std::string& endpointName) :
        m_endpointName{endpointName} {
    m_notifier = std::make_shared<AlexaLauncherNotifier>();
}

/**
 * A helper function to generate string out of Target object.
 * @param target This struct holds target related values such as identifier, name.
 * @return A string object that contains Target content
 */
static std::string generateTargetPrint(const acsdkAlexaLauncherInterfaces::TargetState& targetState) {
    std::string targetPayload =
        " LaunchTarget {  identifier: " + targetState.identifier + "," + " name: " + targetState.name + " } ";
    return "{" + targetPayload + " } ";
}

AlexaLauncherInterface::Response EndpointAlexaLauncherHandler::launchTarget(
    const acsdkAlexaLauncherInterfaces::TargetState& targetState) {
    std::string payload = generateTargetPrint(targetState);
    ConsolePrinter::prettyPrint({"API Name: " + std::string(NAMESPACE),
                                 +"API Version: " + std::string(INTERFACE_VERSION),
                                 +"ENDPOINT: " + m_endpointName,
                                 +"Launch Target PAYLOAD: " + payload});
    AlexaLauncherInterface::Response response;
    bool retVal = notifyTargetCallback(targetState.name);

    if (retVal) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_currentTargetState = targetState;
        lock.unlock();

        notifyObservers(targetState);
        response = {AlexaLauncherInterface::Response::Type::SUCCESS, ""};
    } else {
        response = {AlexaLauncherInterface::Response::Type::NOT_SUPPORTED_IN_CURRENT_MODE,
                    "Unable to launch the target specified."};
    }

    return response;
}

acsdkAlexaLauncherInterfaces::TargetState EndpointAlexaLauncherHandler::getLauncherTargetState() {
    std::lock_guard<std::mutex> lock{m_mutex};
    ACSDK_DEBUG5(LX(__func__));

    return m_currentTargetState;
}

bool EndpointAlexaLauncherHandler::addObserver(
    const std::weak_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_notifier->addWeakPtrObserver(observer);
    ACSDK_DEBUG5(LX(__func__));
    return true;
}

void EndpointAlexaLauncherHandler::removeObserver(
    const std::weak_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_mutex};
    m_notifier->removeWeakPtrObserver(observer);
}

bool EndpointAlexaLauncherHandler::registerLaunchTargetCallback(
    const std::string& targetName,
    LaunchHandlerCallback callback) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_launcherCallbacks.find(targetName);
    if (it != m_launcherCallbacks.end()) {
        ACSDK_ERROR(LX(__func__).d("reason", "target already registered."));
        return false;
    }

    m_launcherCallbacks.emplace(targetName, callback);
    return true;
}

bool EndpointAlexaLauncherHandler::deregisterLaunchTargetCallback(const std::string& targetName) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_launcherCallbacks.find(targetName);
    if (it == m_launcherCallbacks.end()) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "No callback registered for this target."));
        return false;
    }

    m_launcherCallbacks.erase(it);
    return true;
}

bool EndpointAlexaLauncherHandler::notifyTargetCallback(const std::string& targetName) {
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_launcherCallbacks.find(targetName);
    if (it == m_launcherCallbacks.end()) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "No callback registered for this target."));
        return false;
    }

    it->second();
    return true;
}

void EndpointAlexaLauncherHandler::notifyObservers(const acsdkAlexaLauncherInterfaces::TargetState& targetState) {
    ACSDK_DEBUG5(LX(__func__));
    m_notifier->notifyObservers([targetState](const std::shared_ptr<AlexaLauncherObserverInterface>& observer) {
        if (observer) {
            observer->onLauncherTargetChanged(targetState);
        }
    });
}

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK
