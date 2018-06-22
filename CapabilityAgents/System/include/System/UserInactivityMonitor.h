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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_USERINACTIVITYMONITOR_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_USERINACTIVITYMONITOR_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/UserActivityNotifierInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/// This class implementes a @c CapabilityAgent that handles the @c SetEndpoint directive.
class UserInactivityMonitor
        : public avsCommon::sdkInterfaces::UserActivityNotifierInterface
        , public avsCommon::avs::CapabilityAgent {
public:
    /**
     * Create an instance of @c UserInactivityMonitor.
     *
     * @param messageSender The @c MessageSenderInterface for sending events.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @param sendPeriod The period of send events in seconds.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c UserInactivityMonitor.
     */
    static std::shared_ptr<UserInactivityMonitor> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        const std::chrono::milliseconds& sendPeriod = std::chrono::hours(1));

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name UserActivityNotifierInterface functions
    /// @{
    void onUserActive() override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param messageSender The @c MessageSenderInterface for sending events.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @param sendPeriod The period of send events in seconds.
     */
    UserInactivityMonitor(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        const std::chrono::milliseconds& sendPeriod);

    /**
     *
     * Remove the directive (if possible) while invoking callbacks to @c DirectiveHandlerResultInterface.
     *
     * @param info The @c DirectiveInfo we are trying to remove.
     * @param isFailure Boolean flag set to @c true if something went wrong before removing the directive.
     * @param report The report that we will pass to @c setFailed in case @c isFailure is @c true.
     */
    void removeDirectiveGracefully(
        std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info,
        bool isFailure = false,
        const std::string& report = "");

    /// Send inactivity report by comparing to the last time active. We will register this function with the timer.
    void sendInactivityReport();

    /// The @c MessageSender interface to send inactivity event.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /**
     * Time point to keep user inactivity. Access synchronized by @c m_timeMutex, and blocks tracked by @c
     * m_recentUpdateBlocked.
     */
    std::chrono::time_point<std::chrono::steady_clock> m_lastTimeActive;

    /// Time point synchronizer mutex.
    std::mutex m_timeMutex;

    /// Flag for notifiying that @c onUserActive was not able to update @c m_lastTimeActive.
    std::atomic_bool m_recentUpdateBlocked;

    /// Timer for sending events every hour.
    avsCommon::utils::timing::Timer m_eventTimer;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_USERINACTIVITYMONITOR_H_
