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

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <Settings/SettingEventSender.h>

#include "System/ReportStateHandler.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::error;
using namespace settings;

/// String to identify log entries originating from this file.
#define TAG "ReportStateHandler"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// This string holds the namespace for AVS enpointing.
static const std::string REPORT_STATE_NAMESPACE = "System";

/// This string holds the name of the directive that's being sent for report states.
static const std::string REPORT_STATE_DIRECTIVE = "ReportState";

/// Key for pending report value.
static const std::string PENDING_REPORT_STATE_KEY = "pendingReportState";

/// Component name used to store pending value in the misc storage.
static const std::string REPORT_STATE_COMPONENT_NAME = "ReportStateHandler";

/// The report state table name.
static const std::string REPORT_STATE_TABLE = "ReportStateTable";

/// The value we add to the database when a report is pending.
static const std::string PENDING_REPORT_VALUE = "true";

/// This structure represents the report state event metadata.
static const settings::SettingEventMetadata REPORT_STATE_METADATA{REPORT_STATE_NAMESPACE,
                                                                  std::string(),
                                                                  std::string("StateReport"),
                                                                  std::string("states")};

/**
 * Initialize the storage.
 *
 * @param storage The storage to be initialized.
 * @return @c true if it succeeded, false otherwise.
 */
static bool initializeDataBase(storage::MiscStorageInterface& storage) {
    if (!storage.isOpened() && !storage.open() && !storage.isOpened()) {
        ACSDK_DEBUG3(LX(__func__).m("Couldn't open misc database. Creating."));
        if (!storage.createDatabase()) {
            ACSDK_ERROR(LX("initializeDatabaseFailed").d("reason", "Could not create misc database."));
            return false;
        }
    }

    bool tableExists = false;
    if (!storage.tableExists(REPORT_STATE_COMPONENT_NAME, REPORT_STATE_TABLE, &tableExists)) {
        ACSDK_ERROR(LX("initializeDatabaseFailed")
                        .d("reason",
                           "Could not get Capabilities table information misc "
                           "database."));
        return false;
    }

    if (!tableExists) {
        ACSDK_DEBUG3(LX(__func__).m("Table doesn't exist in misc database. Creating new."));
        if (!storage.createTable(
                REPORT_STATE_COMPONENT_NAME,
                REPORT_STATE_TABLE,
                storage::MiscStorageInterface::KeyType::STRING_KEY,
                storage::MiscStorageInterface::ValueType::STRING_VALUE)) {
            ACSDK_ERROR(LX("initializeDatabaseFailed")
                            .d("reason", "Cannot create table")
                            .d("component", REPORT_STATE_COMPONENT_NAME)
                            .d("table", REPORT_STATE_TABLE));
            return false;
        }
    }

    return true;
}

std::unique_ptr<ReportStateHandler> ReportStateHandler::create(
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage,
    const std::vector<StateReportGenerator>& generators) {
    if (!dataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullDataManager"));
        return nullptr;
    }

    if (!exceptionEncounteredSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionEncountered"));
        return nullptr;
    }

    if (!storage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStorage"));
        return nullptr;
    }

    if (generators.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "noReportGenerator"));
        return nullptr;
    }

    if (!initializeDataBase(*storage)) {
        return nullptr;
    }

    auto eventSender = settings::SettingEventSender::create(REPORT_STATE_METADATA, std::move(messageSender));

    bool pendingReport = false;
    if (!storage->tableEntryExists(
            REPORT_STATE_COMPONENT_NAME, REPORT_STATE_TABLE, PENDING_REPORT_STATE_KEY, &pendingReport)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "cannotAccessStorage"));
        return nullptr;
    }

    auto handler = std::unique_ptr<ReportStateHandler>(new ReportStateHandler(
        std::move(dataManager),
        std::move(exceptionEncounteredSender),
        std::move(connectionManager),
        std::move(storage),
        std::move(eventSender),
        generators,
        pendingReport));

    handler->initialize();

    return handler;
}

DirectiveHandlerConfiguration ReportStateHandler::getConfiguration() const {
    return DirectiveHandlerConfiguration{{NamespaceAndName{REPORT_STATE_NAMESPACE, REPORT_STATE_DIRECTIVE},
                                          BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false)}};
}

void ReportStateHandler::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_ERROR(LX("handleDirectiveImmediatelyFailed").d("reason", "nullDirective"));
        return;
    }

    m_executor.execute([this, directive] { handleReportState(*directive); });
}

void ReportStateHandler::handleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    if (!info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirective"));
        return;
    }

    m_executor.execute([this, info] {
        auto ok = handleReportState(*(info->directive));
        if (info->result) {
            if (ok) {
                info->result->setCompleted();
            } else {
                info->result->setFailed("HandleReportStateFailed");
            }
        }
    });
}

void ReportStateHandler::preHandleDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}

void ReportStateHandler::cancelDirective(std::shared_ptr<CapabilityAgent::DirectiveInfo> info) {
    // intentional no-op
}

bool ReportStateHandler::handleReportState(const AVSDirective& info) {
    ACSDK_DEBUG5(LX(__func__));
    // Guarantee directive removal.
    FinallyGuard finally{[this, &info] { removeDirective(info.getMessageId()); }};
    if (info.getName() != REPORT_STATE_DIRECTIVE) {
        ACSDK_ERROR(LX("handleReportStateFailed").d("reason", "unexpectedDirective").d("directive", info.getName()));
        return false;
    }
    m_pendingReport = true;
    m_storage->put(REPORT_STATE_COMPONENT_NAME, REPORT_STATE_TABLE, PENDING_REPORT_STATE_KEY, PENDING_REPORT_VALUE);
    sendReportState();
    return true;
}

ReportStateHandler::ReportStateHandler(
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> dataManager,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage,
    std::unique_ptr<SettingEventSenderInterface> eventSender,
    const std::vector<StateReportGenerator>& generators,
    bool pendingReport) :
        CapabilityAgent{REPORT_STATE_NAMESPACE, std::move(exceptionEncounteredSender)},
        registrationManager::CustomerDataHandler{std::move(dataManager)},
        m_connectionManager{std::move(connectionManager)},
        m_storage{std::move(storage)},
        m_generators{generators},
        m_eventSender{std::move(eventSender)},
        m_pendingReport{pendingReport} {
}

void ReportStateHandler::initialize() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_connectionObserver = SettingConnectionObserver::create([this](bool isConnected) {
        if (isConnected) {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_executor.execute([this] { sendReportState(); });
        }
    });
    m_connectionManager->addConnectionStatusObserver(m_connectionObserver);
}

ReportStateHandler::~ReportStateHandler() {
    m_executor.shutdown();
    m_connectionManager->removeConnectionStatusObserver(m_connectionObserver);
}

void ReportStateHandler::sendReportState() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    ACSDK_DEBUG5(LX(__func__).d("pendingReport", m_pendingReport));
    if (m_pendingReport) {
        json::JsonGenerator jsonGenerator;
        std::set<std::string> states;
        for (auto& generator : m_generators) {
            auto report = generator.generateReport();
            states.insert(report.begin(), report.end());
        }

        jsonGenerator.addMembersArray(REPORT_STATE_METADATA.settingName, states);
        auto statesJson = jsonGenerator.toString();
        if (!m_eventSender->sendStateReportEvent(statesJson).get()) {
            ACSDK_ERROR(LX("sendReportEventFailed").sensitive("state", statesJson));
            return;
        }

#ifdef ACSDK_EMIT_SENSITIVE_LOGS
        ACSDK_DEBUG5(LX(__func__).sensitive("state", statesJson));
#endif
        m_pendingReport = false;
        m_storage->remove(REPORT_STATE_COMPONENT_NAME, REPORT_STATE_TABLE, PENDING_REPORT_STATE_KEY);
    }
}

void system::ReportStateHandler::clearData() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_storage->clearTable(REPORT_STATE_COMPONENT_NAME, REPORT_STATE_TABLE);
}

void ReportStateHandler::addStateReportGenerator(const StateReportGenerator& generator) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_generators.push_back(generator);
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
