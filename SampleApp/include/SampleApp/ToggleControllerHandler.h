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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_TOGGLECONTROLLERHANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_TOGGLECONTROLLERHANDLER_H_

#include <list>

#include <AVSCommon/SDKInterfaces/ToggleController/ToggleControllerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * Sample implementation of an @c ToggleControllerInterface.
 */

class ToggleControllerHandler : public avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface {
public:
    /**
     * Create a ToggleControllerHandler object.
     *
     * @return A pointer to a new ToggleControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<ToggleControllerHandler> create();

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
     * Notify toggle state change to the observers of @c ToggleControllerObserverInterface.
     *
     * @param toggleState The changed toggle state to be notified to the observer.
     * @param cause The change cause represented using @c AlexaStateChangeCauseType.
     * @param observers The list of observer to be notified.
     */
    void notifyObservers(
        const avsCommon::sdkInterfaces::toggleController::ToggleControllerInterface::ToggleState& toggleState,
        avsCommon::sdkInterfaces::AlexaStateChangeCauseType cause,
        std::list<std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface>>
            observers);

    /**
     * Set the toggle state the controller.
     *
     * @param toggleState The toggle state of the controller. @c true indicates ON and @c false as OFF.
     */
    void setToggleState(bool toggleState);

private:
    /**
     * Constructor.
     */
    ToggleControllerHandler();

    /// Current toggle state of the capability.
    bool m_currentToggleState;

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The list of @c ToggleControllerObserverInterface observers that will get notified.
    std::list<std::shared_ptr<avsCommon::sdkInterfaces::toggleController::ToggleControllerObserverInterface>>
        m_observers;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_TOGGLECONTROLLERHANDLER_H_
