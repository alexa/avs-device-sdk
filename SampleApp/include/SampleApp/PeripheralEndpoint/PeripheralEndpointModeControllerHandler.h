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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PERIPHERALENDPOINT_PERIPHERALENDPOINTMODECONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PERIPHERALENDPOINT_PERIPHERALENDPOINTMODECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * An implementation of an @c ModeControllerInterface.
 */
class PeripheralEndpointModeControllerHandler
        : public avsCommon::sdkInterfaces::modeController::ModeControllerInterface {
public:
    /// The mode 'Red' of the controller
    static const std::string MODE_CONTROLLER_MODE_RED;

    /// The mode 'Green' of the controller
    static const std::string MODE_CONTROLLER_MODE_GREEN;

    /// The mode 'Blue' of the controller
    static const std::string MODE_CONTROLLER_MODE_BLUE;

    /**
     * Create a PeripheralEndpointModeControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @param instance The instance name of the capability.
     * @return A pointer to a new @c PeripheralEndpointModeControllerHandler object if it succeeds; otherwise, @c
     * nullptr.
     */
    static std::shared_ptr<PeripheralEndpointModeControllerHandler> create(
        const std::string& endpointName,
        const std::string& instance);

    /// @name ModeControllerInterface methods
    /// @{
    avsCommon::sdkInterfaces::modeController::ModeControllerInterface::ModeControllerConfiguration getConfiguration()
        override;
    std::pair<avsCommon::avs::AlexaResponseType, std::string> setMode(
        const std::string& mode,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<avsCommon::avs::AlexaResponseType, std::string> adjustMode(
        int modeDelta,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<
        avsCommon::avs::AlexaResponseType,
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerInterface::ModeState>>
    getMode() override;
    bool addObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerObserverInterface> observer) override;
    void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerObserverInterface>& observer)
        override;
    /// @}

    /**
     * Set the mode the controller.
     *
     * @param mode Mode of the controller.
     */
    void setMode(const std::string& mode);

private:
    /**
     * Constructor.
     *
     * @param endpointName The name of the endpoint.
     * @param instance The instance name of the capability.
     */
    PeripheralEndpointModeControllerHandler(const std::string& endpointName, const std::string& instance);

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// The instance name of the capability.
    std::string m_instance;

    /// Current mode of the capability.
    std::string m_currentMode;

    /// Supported modes.
    std::vector<std::string> m_modes;

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The list of @c ModeControllerObserverInterface observers that will get notified.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::modeController::ModeControllerObserverInterface>> m_observers;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_PERIPHERALENDPOINT_PERIPHERALENDPOINTMODECONTROLLERHANDLER_H_
