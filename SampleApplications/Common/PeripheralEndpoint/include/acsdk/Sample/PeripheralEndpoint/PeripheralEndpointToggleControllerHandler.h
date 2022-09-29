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

#ifndef ACSDK_SAMPLE_PERIPHERALENDPOINT_PERIPHERALENDPOINTTOGGLECONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_PERIPHERALENDPOINT_PERIPHERALENDPOINTTOGGLECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c ToggleControllerInterface.
 */

class PeripheralEndpointToggleControllerHandler
        : public avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface {
public:
    /**
     * Create a PeripheralEndpointToggleControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @param instance The instance name of the capability.
     * @return A pointer to a new PeripheralEndpointToggleControllerHandler object if it succeeds; otherwise, @c
     * nullptr.
     */
    static std::shared_ptr<PeripheralEndpointToggleControllerHandler> create(
        const std::string& endpointName,
        const std::string& instance);

    /// @name ToggleControllerInterface methods
    /// @{
    std::pair<avsCommon::avs::AlexaResponseType, std::string> setToggleState(
        bool state,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause) override;
    std::pair<
        avsCommon::avs::AlexaResponseType,
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface::ToggleState>>
    getToggleState() override;
    bool addObserver(std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface>
                         observer) override;
    void removeObserver(
        const std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface>& observer)
        override;
    /// @}

    /**
     * Set the toggle state the controller.
     *
     * @param toggleState The toggle state of the controller. @c true indicates ON and @c false as OFF.
     */
    void setToggleState(bool toggleState);

private:
    /**
     * Constructor.
     *
     * @param endpointName The name of the endpoint.
     * @param instance The instance name of the capability.
     */
    PeripheralEndpointToggleControllerHandler(const std::string& endpointName, const std::string& instance);

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// The instance name of the capability.
    std::string m_instance;

    /// Current toggle state of the capability.
    bool m_currentToggleState;

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The list of @c ToggleControllerObserverInterface observers that will get notified.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface>>
        m_observers;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_PERIPHERALENDPOINT_PERIPHERALENDPOINTTOGGLECONTROLLERHANDLER_H_
