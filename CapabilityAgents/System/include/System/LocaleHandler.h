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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_LOCALEHANDLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_LOCALEHANDLER_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingEventMetadata.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implements a @c CapabilityAgent that handles the @c SetLocale directive.
 */
class LocaleHandler : public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Create an instance of @c LocaleHandler.
     *
     * @param exceptionSender The interface that sends exceptions.
     * @param localeSetting The container that implements the locale.
     * @return A new instance of @c LocaleHandler if the creation succeeds; @c nullptr otherwise.
     */
    static std::unique_ptr<LocaleHandler> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<settings::LocalesSetting> localeSetting);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /**
     * Get the locale events metadata.
     *
     * @return The locale events metadata.
     */
    static settings::SettingEventMetadata getLocaleEventsMetadata();

private:
    /**
     * An implementation of this directive handling function, which may be called by the internal executor.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void executeHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info);

    /**
     * A utility function to simplify calling the @c ExceptionEncounteredSender.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param errorMessage The error message we will send to AVS.
     */
    void sendProcessingDirectiveException(
        std::shared_ptr<CapabilityAgent::DirectiveInfo> info,
        const std::string& errorMessage);

    /**
     * A helper function to handle the setLocale directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     */
    void handleSetLocale(std::shared_ptr<CapabilityAgent::DirectiveInfo> info);

    /**
     * Constructor.
     *
     * @param exceptionSender The interface that sends exceptions.
     * @param localeSetting The container that implements the locale.
     */
    LocaleHandler(
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<settings::LocalesSetting> localeSetting);

    /// The locale setting.
    std::shared_ptr<settings::LocalesSetting> m_localeSetting;

    /// The @c Executor which queues up operations from asynchronous API calls.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_LOCALEHANDLER_H_
