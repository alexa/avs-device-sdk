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

#ifndef ACSDKINPUTCONTROLLER_SRC_INPUTCONTROLLERCAPABILITYAGENT_H_
#define ACSDKINPUTCONTROLLER_SRC_INPUTCONTROLLERCAPABILITYAGENT_H_

#include <memory>
#include <string>
#include <unordered_set>

#include <acsdkInputControllerInterfaces/InputControllerHandlerInterface.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace acsdkInputController {

/**
 * The Input Controller Capability Agent provides an implementation for a client to interface with the
 * Alexa.InputController API.
 *
 * @see https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/inputcontroller.html
 *
 * AVS sends SelectInput directives to the device to switch the input.
 *
 */
class InputControllerCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface {
public:
    /// Alias for brevity
    using InputFriendlyNameConfigurations =
        acsdkInputControllerInterfaces::InputControllerHandlerInterface::InputFriendlyNameType;

    /**
     * Destructor.
     */
    virtual ~InputControllerCapabilityAgent() = default;

    /**
     * Creates an instance of the @c InputControllerCapabilityAgent.
     *
     * @param handler The handler for InputController input updates.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     * @return A @c std::shared_ptr to the new @c InputControllerCapabilityAgent instance, a nullptr if failed.
     */
    static std::shared_ptr<InputControllerCapabilityAgent> create(
        const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface>& handler,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

    /// @name CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param handler The handler for InputController input updates.
     * @param exceptionSender The object to use for sending AVS Exception messages.
     */
    InputControllerCapabilityAgent(
        const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface>& handler,
        const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender);

    /**
     * Helper function to process the incoming directive
     *
     * @param info Directive to be processed.
     * @param[out] errMessage Error message associated to failure to process the directive
     * @param[out] type Error type associated to failure to process the directive
     * @return A bool indicating the success of processing the directive
     */
    bool executeHandleDirectiveHelper(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        std::string* errMessage,
        avsCommon::avs::ExceptionErrorType* type);

    /// Set of capability configurations that will get published using Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The object to handle input change events.
    const std::shared_ptr<acsdkInputControllerInterfaces::InputControllerHandlerInterface> m_inputControllerHandler;

    /// The configuration of the inputs that is read from the configuration file.
    InputFriendlyNameConfigurations m_inputConfigurations;

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkInputController
}  // namespace alexaClientSDK

#endif  // ACSDKINPUTCONTROLLER_SRC_INPUTCONTROLLERCAPABILITYAGENT_H_
