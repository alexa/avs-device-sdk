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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REPORTSTATEHANDLER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REPORTSTATEHANDLER_H_

#include <memory>
#include <string>
#include <thread>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <Settings/SettingEventSenderInterface.h>
#include <Settings/SettingConnectionObserver.h>

#include "System/StateReportGenerator.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class implements a @c CapabilityAgent that handles the @c ReportState directive.
 */
class ReportStateHandler
        : public avsCommon::avs::CapabilityAgent
        , public registrationManager::CustomerDataHandler {
public:
    /**
     * Create an instance of @c ReportStateHandler.
     *
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param messageSender The delivery service for the AVS events.
     * @param settingStorage The setting storage object.
     * @param generators A collection of report generators. This collection should have at least one generator.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c ReportStateHandler.
     */
    static std::unique_ptr<ReportStateHandler> create(
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> settingStorage,
        const std::vector<StateReportGenerator>& generators);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CustomerDataHandler functions.
    /// @{
    void clearData() override;
    /// @}

    /**
     * Destructor.
     */
    ~ReportStateHandler();

    /**
     * Adds a new @c StateReportGenerator.
     *
     * @param generator The state report generator.
     */
    void addStateReportGenerator(const StateReportGenerator& generator);

private:
    /**
     * Constructor.
     *
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @param exceptionEncounteredSender The interface that sends exceptions.
     * @param connectionManager An @c AVSConnectionManagerInterface instance to listen for connection status updates.
     * @param storage The setting storage object.
     * @param eventSender Object used to send the report to AVS.
     * @param generators A collection of report generators. This collection should have at least one generator.
     * @param pendingReport A flag indicating if there is a report pending.
     */
    ReportStateHandler(
        std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage,
        std::unique_ptr<settings::SettingEventSenderInterface> eventSender,
        const std::vector<StateReportGenerator>& generators,
        bool pendingReport);

    /**
     * Function called to handle a report state directive.
     *
     * @param directive The report state directive.
     */
    bool handleReportState(const avsCommon::avs::AVSDirective& directive);

    /**
     * Function that sends the report state.
     */
    void sendReportState();

    /**
     * Initialization after construction.
     */
    void initialize();

private:
    /// Synchronize access since we use members from multiple threads
    std::mutex m_stateMutex;

    /// The @c Executor which queues up operations from asynchronous API calls.
    avsCommon::utils::threading::Executor m_executor;

    /// The AVS connection manager object.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connectionManager;

    /// The storage object.
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_storage;

    /// The generator used to retrieve reports from different settings manager.
    std::vector<StateReportGenerator> m_generators;

    /// Object used to send events to AVS.
    std::unique_ptr<settings::SettingEventSenderInterface> m_eventSender;

    /// The connection observer.
    std::shared_ptr<settings::SettingConnectionObserver> m_connectionObserver;

    /// Flag indicating whether there is a pending report.
    bool m_pendingReport;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_REPORTSTATEHANDLER_H_
