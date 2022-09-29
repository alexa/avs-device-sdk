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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXALAUNCHERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXALAUNCHERHANDLER_H_

#include <list>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <acsdkAlexaLauncherInterfaces/AlexaLauncherInterface.h>
#include <acsdkAlexaLauncherInterfaces/AlexaLauncherObserverInterface.h>
#include <acsdkAlexaLauncherInterfaces/AlexaLauncherTargetState.h>
#include <acsdk/Notifier/Notifier.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaLauncherInterface.
 */

class EndpointAlexaLauncherHandler : public acsdkAlexaLauncherInterfaces::AlexaLauncherInterface {
public:
    /// Alias for a callback function for launching target.
    using LaunchHandlerCallback = std::function<void(void)>;

    /**
     * Create a EndpointAlexaLauncherHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new EndpointAlexaLauncherHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaLauncherHandler> create(const std::string& endpointName);

    /// @name AlexaLauncherInterface methods
    /// @{
    acsdkAlexaLauncherInterfaces::AlexaLauncherInterface::Response launchTarget(
        const acsdkAlexaLauncherInterfaces::TargetState& targetState) override;
    acsdkAlexaLauncherInterfaces::TargetState getLauncherTargetState() override;
    bool addObserver(
        const std::weak_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface>& observer) override;
    void removeObserver(
        const std::weak_ptr<acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface>& observer) override;
    /// @}

    /**
     * Registers a callback for a given launch target.
     * @param targetName Name of the target for this callback.
     * @param callback Method to call when this target is launched by user.
     * @return True if successfully registered.
     */
    bool registerLaunchTargetCallback(const std::string& targetName, LaunchHandlerCallback callback);

    /**
     * De-registers a callback for a given launch target.
     * @param targetName Name of the target for this callback.
     * @return True if successfully de-registered.
     */
    bool deregisterLaunchTargetCallback(const std::string& targetName);

private:
    /// Alias for Notifier based on @c AlexaLauncherObserverInterface.
    using AlexaLauncherNotifier = notifier::Notifier<acsdkAlexaLauncherInterfaces::AlexaLauncherObserverInterface>;

    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaLauncherHandler(const std::string& endpointName);

    /**
     * Helper function to notify target state change to the observers of @c AlexaLauncherObserverInterface.
     *
     * @param targetState The changed target state to be notified to the observer.
     */
    void notifyObservers(const acsdkAlexaLauncherInterfaces::TargetState& targetState);

    /**
     * Notify registered callback that user has requested to launch the specific target.
     * @param targetName name of the target to be launched.
     * @return True if this method was successfully able to invoke a registered callback.
     */
    bool notifyTargetCallback(const std::string& targetName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// Current target state of the endpoint.
    acsdkAlexaLauncherInterfaces::TargetState m_currentTargetState;

    /// A dictionary of all launch target callbacks indexed by the target names.
    std::unordered_map<std::string, LaunchHandlerCallback> m_launcherCallbacks;

    /// Pointer to the AlexaLauncher notifier.
    std::shared_ptr<AlexaLauncherNotifier> m_notifier;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXALAUNCHERHANDLER_H_
