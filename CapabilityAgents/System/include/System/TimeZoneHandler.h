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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_TIMEZONEHANDLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_TIMEZONEHANDLER_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/SystemTimeZoneInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <Settings/DeviceSettingsManager.h>
#include <Settings/SettingEventMetadata.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implements a @c CapabilityAgent that handles the @c SetTimeZone directive.
 */
class TimeZoneHandler : public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Create an instance of @c TimeZoneHandler.
     *
     * @param timeZoneSetting The timezone setting set by this handler.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c TimeZoneHandler.
     */
    static std::unique_ptr<TimeZoneHandler> create(
        std::shared_ptr<settings::TimeZoneSetting> timeZoneSetting,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Gets the timezone events metadata.
     *
     * @return The timezone event metadata.
     */
    static settings::SettingEventMetadata getTimeZoneMetadata();

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

private:
    /**
     * An implementation of this directive handling function, which may be called by the internal executor.
     *
     * @param info The Directive information.
     */
    void executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info);

    /**
     * A utility function to simplify calling the @c ExceptionEncounteredSender.
     *
     * @param directive The AVS Directive which resulted in the exception.
     * @param errorMessage The error message we will send to AVS.
     */
    void sendProcessingDirectiveException(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const std::string& errorMessage);

    /**
     * A helper function to handle the setTimeZone directive.
     *
     * @param directive The AVS Directive.
     * @param payload The payload containing the timezone data fields.
     * @return Whether the directive processing was successful.
     */
    bool handleSetTimeZone(
        const std::shared_ptr<avsCommon::avs::AVSDirective>& directive,
        const rapidjson::Document& payload);

    /**
     * Constructor.
     *
     * @param timeZoneSetting The timezone setting set by this handler.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     */
    TimeZoneHandler(
        std::shared_ptr<settings::TimeZoneSetting> timeZoneSetting,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /// The timezone setting
    std::shared_ptr<settings::TimeZoneSetting> m_timeZoneSetting;

    /// The @c Executor which queues up operations from asynchronous API calls.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_TIMEZONEHANDLER_H_
