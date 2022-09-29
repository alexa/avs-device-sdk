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

#ifndef ACSDK_SAMPLE_ENDPOINT_DEFAULTENDPOINT_DEFAULTENDPOINTMODECONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_DEFAULTENDPOINT_DEFAULTENDPOINTMODECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/ModeController/ModeControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * An implementation of a @c ModeControllerInterface.
 */
class DefaultEndpointModeControllerHandler : public avsCommon::sdkInterfaces::modeController::ModeControllerInterface {
public:
    /// The 'Fan Only' mode of the controller.
    static const std::string MODE_CONTROLLER_MODE_FAN_ONLY;

    /// The friendly name of 'Fan Only' mode.
    static const std::string MODE_CONTROLLER_MODE_FAN_ONLY_FRIENDLY_NAME;

    /// The 'HEAT' mode of the controller
    static const std::string MODE_CONTROLLER_MODE_HEAT;

    /// The friendly name of 'HEAT' mode.
    static const std::string MODE_CONTROLLER_MODE_HEAT_FRIENDLY_NAME;

    /// The 'Cool' mode of the controller.
    static const std::string MODE_CONTROLLER_MODE_COOL;

    /// The friendly name of 'Cool' mode.
    static const std::string MODE_CONTROLLER_MODE_COOL_FRIENDLY_NAME;

    /**
     * Create a DefaultEndpointModeControllerHandler object.
     *
     * @param instance The instance name of the capability.
     * @return A pointer to a new @c DefaultEndpointModeControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<DefaultEndpointModeControllerHandler> create(const std::string& instance);

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

private:
    /**
     * Constructor.
     *
     * @param instance The instance name of the capability.
     */
    DefaultEndpointModeControllerHandler(const std::string& instance);

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

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_DEFAULTENDPOINT_DEFAULTENDPOINTMODECONTROLLERHANDLER_H_
