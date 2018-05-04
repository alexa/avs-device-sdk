/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_

#include <atomic>
#include <memory>

#include "InteractionManager.h"

namespace alexaClientSDK {
namespace sampleApp {

/// Observes user input from the console and notifies the interaction manager of the user's intentions.
class UserInputManager
        : public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::CapabilitiesObserverInterface {
public:
    /**
     * Create a UserInputManager.
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @return Returns a new @c UserInputManager, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<UserInputManager> create(std::shared_ptr<InteractionManager> interactionManager);

    /**
     * Processes user input until a quit command or a device reset is triggered.
     */
    void run();

private:
    /**
     * Constructor.
     */
    UserInputManager(std::shared_ptr<InteractionManager> interactionManager);

    /**
     * Implement speaker control options.
     */
    void controlSpeaker();

    /**
     * Implement device reset confirmation.
     *
     * @return @c true if device was reset; otherwise, return @c false.
     */
    bool confirmReset();

    /// @name AuthObserverInterface Function
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
    /// @}

    /// @name CapabilitiesObserverInterface Function
    /// @{
    void onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State newState,
        CapabilitiesObserverInterface::Error newError) override;
    /// @}

    /// The main interaction manager that interfaces with the SDK.
    std::shared_ptr<InteractionManager> m_interactionManager;

    /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
    /// the app.
    std::atomic_bool m_limitedInteraction;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_
