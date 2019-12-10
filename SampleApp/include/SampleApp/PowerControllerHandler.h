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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_POWERCONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_POWERCONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/PowerController/PowerControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * An implementation of an @c PowerControllerInterface.
 */
class PowerControllerHandler : public avsCommon::sdkInterfaces::powerController::PowerControllerInterface {
public:
    /**
     * Create a PowerControllerHandler object.
     *
     * @return A pointer to a new PowerControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<PowerControllerHandler> create();

    /// @name PowerControllerInterface methods
    /// @{
    std::pair<avsCommon::avs::AlexaResponseType, std::string> setPowerState(
        bool state,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<
        avsCommon::avs::AlexaResponseType,
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::powerController::PowerControllerInterface::PowerState>>
    getPowerState() override;
    bool addObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface> observer) override;
    void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface>& observer)
        override;
    /// @}

    /**
     * Notify power state change to the observers of @c PowerControllerObserverInterface.
     *
     * @param powerState The changed power state to be notified to the observer.
     * @param cause The change cause represented using @c AlexaStateChangeCauseType.
     */
    void executeNotifyObservers(
        const avsCommon::sdkInterfaces::powerController::PowerControllerInterface::PowerState& powerState,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause);

    /**
     * Set the power state of the controller.
     *
     * @param powerState The power state of the controller. @c true indicates ON and @c false as OFF.
     */
    void setPowerState(bool powerState);

private:
    /**
     * Constructor.
     */
    PowerControllerHandler();

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// Current power state of the endpoint.
    bool m_currentPowerState;

    /// The list of @c PowerControllerObserverInterface observers that will get notified.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::powerController::PowerControllerObserverInterface>> m_observers;

    /**
     * Executor to notify the observers.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_POWERCONTROLLERHANDLER_H_
