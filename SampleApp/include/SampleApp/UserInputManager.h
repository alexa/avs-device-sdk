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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_

#include <atomic>
#include <memory>

#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
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
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     * @param defaultEndpointId The @c EndpointIdentifier of the default endpoint.
     * @return Returns a new @c UserInputManager, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<UserInputManager> create(
        std::shared_ptr<InteractionManager> interactionManager,
        std::shared_ptr<ConsoleReader> consoleReader,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& defaultEndpointId);

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
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @param consoleReader The @c ConsoleReader to read inputs from console.
     * @param localeAssetsManager The @c LocaleAssetsManagerInterface that provides the supported locales.
     * @param defaultEndpointId The @c EndpointIdentifier of the default endpoint.
     */
    UserInputManager(
        std::shared_ptr<InteractionManager> interactionManager,
        std::shared_ptr<ConsoleReader> consoleReader,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> localeAssetsManager,
        const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& defaultEndpointId);

    /**
     * Reads an input from the console.  This is a blocking call until an input is read from the console or if m_restart
     * flag is set.  This should only be used in the root level of the menu because we only want to return at the root.
     *
     * @param[out] inputChar A pointer to input from the console.
     * @return Returns @c true if a character is read from console.  @c false if m_restart flag is set.
     */
    bool readConsoleInput(char* input);

#ifdef ENABLE_COMMS
    /**
     * Send dtmf tones if input from console are valid dtmf tones.
     * @param dtmfTones dtmf tones from user input.
     * @return @c true if dtmf tones are sent; otherwise, return @c false.
     */
    bool sendDtmf(const std::string& dtmfTones);
#endif

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

#ifdef ENABLE_MCC
    /**
     * Implement meeting control options.
     */
    void controlMeeting();
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
     * Dynamic endpoint modification Menu.
     */
    void dynamicEndpointModificationMenu();

    /**
     * Endpoint Controller menu.
     */
    void endpointControllerMenu();

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

    /// @name CapabilitiesObserverInterface Methods
    void onCapabilitiesStateChange(
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
        avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
        const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) override;
    /// }

    /**
     * The diagnostics menu.
     */
    void diagnosticsMenu();

    /**
     * The device properties menu.
     */
    void devicePropertiesMenu();

    /**
     * The device protocol tracer menu.
     */
    void deviceProtocolTracerMenu();

    /**
     * The audio injection menu.
     */
    void audioInjectionMenu();

    /// The main interaction manager that interfaces with the SDK.
    std::shared_ptr<InteractionManager> m_interactionManager;

    /// The @c ConsoleReader to read input from console.
    std::shared_ptr<ConsoleReader> m_consoleReader;

    /// The @c LocaleAssetsManagerInterface that provides the supported locales.
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_localeAssetsManager;

    /// The @c EndpointIdentifier of the default endpoint.
    const avsCommon::sdkInterfaces::endpoints::EndpointIdentifier& m_defaultEndpointId;

    /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
    /// the app.
    std::atomic_bool m_limitedInteraction;

    /// Flag to indicate that the @c run() should stop and return @c SampleAppReturnCode::RESTART.
    std::atomic_bool m_restart;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_USERINPUTMANAGER_H_
