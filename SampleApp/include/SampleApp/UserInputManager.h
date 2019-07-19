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

#include <RegistrationManager/RegistrationObserverInterface.h>

#include "ConsoleReader.h"
#include "InteractionManager.h"
#include "SampleApplicationReturnCodes.h"

namespace alexaClientSDK {
namespace sampleApp {

/// Observes user input from the console and notifies the interaction manager of the user's intentions.
class UserInputManager
        : public avsCommon::sdkInterfaces::AuthObserverInterface
        , public avsCommon::sdkInterfaces::CapabilitiesObserverInterface
        , public registrationManager::RegistrationObserverInterface {
public:
    /**
     * Create a UserInputManager.
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @return Returns a new @c UserInputManager, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<UserInputManager> create(
        std::shared_ptr<InteractionManager> interactionManager,
        std::shared_ptr<ConsoleReader> consoleReader);

    /**
     * Processes user input until a quit command or a device reset is triggered.
     *
     * @return Returns a @c SampleAppReturnCode.
     */
    SampleAppReturnCode run();

    /// @name RegistrationObserverInterface Functions
    /// @{
    void onLogout() override;
    /// @}

private:
    /**
     * Constructor.
     */
    UserInputManager(
        std::shared_ptr<InteractionManager> interactionManager,
        std::shared_ptr<ConsoleReader> consoleReader);

    /**
     * Reads an input from the console.  This is a blocking call until an input is read from the console or if m_restart
     * flag is set.  This should only be used in the root level of the menu because we only want to return at the root.
     *
     * @param[out] inputChar A pointer to input from the console.
     * @return Returns @c true if a character is read from console.  @c false if m_restart flag is set.
     */
    bool readConsoleInput(char* input);

    /**
     * Implement speaker control options.
     */
    void controlSpeaker();

#ifdef ENABLE_PCC
    /**
     * Implement phone control options.
     */
    void controlPhone();
#endif

    /**
     * Implement device reset confirmation.
     *
     * @return @c true if device was reset; otherwise, return @c false.
     */
    bool confirmReset();

    /**
     * Settings menu.
     */
    void settingsMenu();

    /**
     * Prompt the user to confirm before initiating re-authorization of the device.
     *
     * @return @c true if re-authorization was confirmed and initiated.
     */
    bool confirmReauthorizeDevice();

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

    /// The @c ConsoleReader to read input from console.
    std::shared_ptr<ConsoleReader> m_consoleReader;

    /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
    /// the app.
    std::atomic_bool m_limitedInteraction;

    /// Flag to indicate that the @c run() should stop and return @c SampleAppReturnCode::RESTART.
    std::atomic_bool m_restart;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_
