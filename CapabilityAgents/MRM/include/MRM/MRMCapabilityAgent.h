/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMCAPABILITYAGENT_H_

#include <memory>
#include <string>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/UserInactivityMonitorObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "MRMHandlerInterface.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace mrm {

/**
 * Implementation of an MRM Capability Agent.
 */
class MRMCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public avsCommon::sdkInterfaces::UserInactivityMonitorObserverInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<MRMCapabilityAgent> {
public:
    /**
     * Creates an instance of this Capability Agent.
     *
     * @param handler The MRM Handler, which handles all MRM-specific implementation.
     * @param speakerManager An object which allows us to detect changes to device Speakers.
     * @param userInactivityMonitor An object which allows us to know about general user activity with the client.
     * @param exceptionEncounteredSender An object which may send System.ExceptionEncountered Events to AVS if needed.
     * @return A pointer to an object of this type, or nullptr if there were problems during construction.
     */
    static std::shared_ptr<MRMCapabilityAgent> create(
        std::unique_ptr<MRMHandlerInterface> mrmHandler,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Destructor.
     */
    ~MRMCapabilityAgent();

    /// @name Overridden CapabilityAgent methods.
    /// @{
    void preHandleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<DirectiveInfo> info) override;
    /// @}

    /// @name Overridden DirectiveHandlerInterface methods (which CapabilityAgent derives from).
    /// @{
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    /// @}

    /// @name Overridden SpeakerManagerObserverInterface methods.
    /// @{
    void onSpeakerSettingsChanged(
        const avsCommon::sdkInterfaces::SpeakerManagerObserverInterface::Source& source,
        const avsCommon::sdkInterfaces::SpeakerInterface::Type& type,
        const avsCommon::sdkInterfaces::SpeakerInterface::SpeakerSettings& settings) override;
    /// @}

    /// @name Overridden UserInactivityMonitorObserverInterface methods.
    /// @{
    void onUserInactivityReportSent() override;
    /// @}

    /// @name Overridden CapabilityConfigurationInterface methods.
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name Overridden RequiresShutdown methods.
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * Returns the string representation of the version of this MRM implementation.
     *
     * @return The string representation of the version of this MRM implementation.
     */
    std::string getVersionString() const;

private:
    /**
     * Constructor.
     *
     * @param handler The MRM Handler, which handles all MRM-specific implementation.
     * @param speakerManager An object which allows us to detect changes to device Speakers.
     * @param userActivityMonitor An object which allows us to know about general user activity with the client.
     * @param exceptionEncounteredSender An object which may send System.ExceptionEncountered Events to AVS if needed.
     */
    MRMCapabilityAgent(
        std::unique_ptr<MRMHandlerInterface> handler,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> userInactivityMonitor,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * @name Executor Thread Functions
     *
     * These functions (and only these functions) are called by @c m_executor on a single worker thread.  All other
     * functions in this class can be called asynchronously, and pass data to the @c Executor thread through parameters
     * to lambda functions.  No additional synchronization is needed.
     */
    /// @{

    /**
     * This function handles an incoming Directive.
     *
     * @param info The DirectiveInfo to be handled.
     */
    void executeHandleDirectiveImmediately(std::shared_ptr<DirectiveInfo> info);

    /**
     * This function handles when a speaker setting has changed.
     *
     * @param type The type of the Speaker which changed.
     */
    void executeOnSpeakerSettingsChanged(const avsCommon::sdkInterfaces::SpeakerInterface::Type& type);

    /**
     * This function handles when a System.UserInactivityReport has been sent to AVS.
     */
    void executeOnUserInactivityReportSent();

    /// @}

    /// Our MRM Handler.
    std::unique_ptr<MRMHandlerInterface> m_mrmHandler;
    /// The Speaker Manager.
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> m_speakerManager;
    /// The User Inactivity Monitor.
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface> m_userInactivityMonitor;
    /// The @c Executor which queues up operations from asynchronous API calls.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace mrm
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMCAPABILITYAGENT_H_