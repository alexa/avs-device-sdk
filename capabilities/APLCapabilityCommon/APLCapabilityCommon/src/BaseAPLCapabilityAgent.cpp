/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     https://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <chrono>
#include <ostream>
#include <tuple>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointStringBuilder.h>
#include <AVSCommon/Utils/Metrics/DataPointDurationBuilder.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>

#include "acsdk/APLCapabilityCommon/APLPayloadParser.h"
#include "acsdk/APLCapabilityCommon/BaseAPLCapabilityAgent.h"

namespace alexaClientSDK {
namespace aplCapabilityCommon {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::timing;

/// String to identify log entries originating from this file.
#define TAG "BaseAPLCapabilityAgent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Identifier for the grantedExtensions in presentationSession
static const char PRESENTATION_SESSION_GRANTEDEXTENSIONS[] = "grantedExtensions";

/// Identifier for the autoInitializedExtensions in presentationSession
static const char PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS[] = "autoInitializedExtensions";

//// Identifier for the uri in grantedExtensions or autoInitializedExtensions
static const char PRESENTATION_SESSION_URI[] = "uri";

/// Identifier for the settings in autoInitializedExtensions
static const char PRESENTATION_SESSION_SETTINGS[] = "settings";

/// The key in our config file to set the minimum time in ms between reporting proactive state report events
static const char MIN_STATE_REPORT_INTERVAL_KEY[] = "minStateReportIntervalMs";

/// The key in our config file to set the time in ms between proactive state report checks - 0 disables the feature
static const char STATE_REPORT_CHECK_INTERVAL_KEY[] = "stateReportCheckIntervalMs";

/// StaticRequestToken value for providing Change Report state
static const auto PROACTIVE_STATE_REQUEST_TOKEN = 0;

/// The name for UserEvent event.
static const char USER_EVENT[] = "UserEvent";

/// The name for LoadIndexListData event.
static const char LOAD_INDEX_LIST_DATA[] = "LoadIndexListData";

/// The name for LoadTokenListData event.
static const char LOAD_TOKEN_LIST_DATA[] = "LoadTokenListData";

/// The name for RuntimeError event.
static const char RUNTIME_ERROR[] = "RuntimeError";

/// Identifier for the presentationToken's sent in a RenderDocument directive
static const char PRESENTATION_TOKEN[] = "presentationToken";

/// Identifier for the presentationSession sent in a RenderDocument directive
static const char PRESENTATION_SESSION_FIELD[] = "presentationSession";

/// Identifier for the commands sent in a RenderDocument directive
static const char COMMANDS_FIELD[] = "commands";

/// Tag for finding the visual context information sent from the runtime as part of event context.
static const char VISUAL_CONTEXT_NAME[] = "RenderedDocumentState";

/// Dynamic index list data source type
static const char DYNAMIC_INDEX_LIST[] = "dynamicIndexList";

/// Dynamic token list data source type
static const char DYNAMIC_TOKEN_LIST[] = "dynamicTokenList";

/// Default minimum interval between state reports
static std::chrono::milliseconds DEFAULT_MIN_STATE_REPORT_INTERVAL_MS = std::chrono::milliseconds(600);

/// Default interval between proactive state report checks - disabled by default
static std::chrono::milliseconds DEFAULT_STATE_REPORT_CHECK_INTERVAL_MS = std::chrono::milliseconds(0);

/// The keys used in @c RenderedDocumentState payload
static const char TOKEN_KEY[] = "token";
static const char VERSION_KEY[] = "version";
static const char VISUAL_CONTEXT_KEY[] = "componentsVisibleOnScreen";
static const char DATASOURCE_CONTEXT_KEY[] = "dataSources";

/// The keys used in @c UserEvent payload
static const char ARGUMENTS_KEY[] = "arguments";
static const char SOURCE_KEY[] = "source";
static const char COMPONENTS_KEY[] = "components";

/// Initializes the object by reading the values from configuration.
bool BaseAPLCapabilityAgent::initialize() {
    auto configurationRoot = ConfigurationNode::getRoot()[getConfigurationRootKey()];

    configurationRoot.getDuration<std::chrono::milliseconds>(
        MIN_STATE_REPORT_INTERVAL_KEY, &m_minStateReportInterval, DEFAULT_MIN_STATE_REPORT_INTERVAL_MS);

    configurationRoot.getDuration<std::chrono::milliseconds>(
        STATE_REPORT_CHECK_INTERVAL_KEY, &m_stateReportCheckInterval, DEFAULT_STATE_REPORT_CHECK_INTERVAL_MS);

    if (m_visualStateProvider) {
        ACSDK_DEBUG3(LX(__func__).d("visualStateProvider", "On"));
        m_contextManager->setStateProvider(m_visualContextHeader, shared_from_this());
    }

    if (m_stateReportCheckInterval.count() == 0) {
        ACSDK_DEBUG0(LX(__func__).m("Proactive state report timer disabled"));
    } else {
        if (m_stateReportCheckInterval < m_minStateReportInterval) {
            ACSDK_WARN(
                LX(__func__).m("State check interval cannot be less than minimum reporting interval, setting check "
                               "interval to minimum report interval"));
            m_stateReportCheckInterval = m_minStateReportInterval;
        }

        ACSDK_DEBUG0(LX(__func__)
                         .d("minStateReportIntervalMs", m_minStateReportInterval.count())
                         .d("stateReportCheckIntervalMs", m_stateReportCheckInterval.count()));

        m_proactiveStateTimer.start(
            m_stateReportCheckInterval,
            Timer::PeriodType::ABSOLUTE,
            Timer::FOREVER,
            std::bind(&BaseAPLCapabilityAgent::proactiveStateReport, this));
    }

    return true;
}

void BaseAPLCapabilityAgent::setExecutor(
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor>& executor) {
    ACSDK_WARN(LX(__func__).d("reason", "should be called in test only"));
    m_executor = executor;
}

std::shared_ptr<alexaClientSDK::avsCommon::utils::threading::Executor> BaseAPLCapabilityAgent::getExecutor() {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor;
}

void BaseAPLCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void BaseAPLCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("preHandleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }
}

void BaseAPLCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    // Must remain on the very fist line for accurate telemetry
    m_renderReceivedTime = std::chrono::steady_clock::now();
    ACSDK_DEBUG5(LX(__func__));
    if (!info || !info->directive) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        handleUnknownDirective(info);
        return;
    }

    switch (getDirectiveType(info)) {
        case DirectiveType::RENDER_DOCUMENT:
            handleRenderDocumentDirective(info);
            break;
        case DirectiveType::SHOW_DOCUMENT:
            handleShowDocumentDirective(info);
            setHandlingCompleted(info);
            break;
        case DirectiveType::EXECUTE_COMMAND:
            handleExecuteCommandDirective(info);
            break;
        case DirectiveType::DYNAMIC_INDEX_DATA_SOURCE_UPDATE:
            handleDynamicListDataDirective(info, DYNAMIC_INDEX_LIST);
            break;
        case DirectiveType::DYNAMIC_TOKEN_DATA_SOURCE_UPDATE:
            handleDynamicListDataDirective(info, DYNAMIC_TOKEN_LIST);
            break;
        default:
            handleUnknownDirective(info);
            break;
    }
}

void BaseAPLCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

DirectiveHandlerConfiguration BaseAPLCapabilityAgent::getConfiguration() const {
    return getAPLDirectiveConfiguration();
}

void BaseAPLCapabilityAgent::clearExecuteCommands(
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    const bool markAsFailed) {
    m_executor->submit(
        [this, token, markAsFailed]() { executeClearExecuteCommands("User exited", token, markAsFailed); });
}

void BaseAPLCapabilityAgent::executeClearExecuteCommands(
    const std::string& reason,
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    const bool markAsFailed) {
    ACSDK_DEBUG5(LX(__func__).d("reason", reason));
    if (!(m_lastExecuteCommandTokenAndDirective.first.empty()) && m_lastExecuteCommandTokenAndDirective.second &&
        m_lastExecuteCommandTokenAndDirective.second->result) {
        if (!token.empty() && m_lastExecuteCommandTokenAndDirective.first != token) {
            ACSDK_ERROR(LX(__func__).d(
                "reason", "presentationToken in the last ExecuteCommand does not match with the provided token."));
            return;
        }
        if (markAsFailed) {
            m_lastExecuteCommandTokenAndDirective.second->result->setFailed(reason);
        } else {
            m_lastExecuteCommandTokenAndDirective.second->result->setCompleted();
        }
    }

    m_lastExecuteCommandTokenAndDirective.first.clear();
}

BaseAPLCapabilityAgent::BaseAPLCapabilityAgent(
    const std::string& avsNamespace,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    const std::string& APLVersion,
    std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::VisualStateProviderInterface> visualStateProvider) :
        CapabilityAgent{avsNamespace, exceptionSender},
        RequiresShutdown{"BaseAPLCapabilityAgent:" + avsNamespace},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_visualStateProvider{visualStateProvider},
        m_APLVersion{APLVersion},
        m_metricRecorder{metricRecorder},
        m_lastReportTime{std::chrono::steady_clock::now()},
        m_minStateReportInterval{DEFAULT_MIN_STATE_REPORT_INTERVAL_MS},
        m_stateReportPending{false},
        m_documentRendered{false},
        m_presentationSession{},
        m_avsNamespace{avsNamespace},
        m_visualContextHeader{avsNamespace, VISUAL_CONTEXT_NAME} {
    m_executor = std::make_shared<alexaClientSDK::avsCommon::utils::threading::Executor>();
}

void BaseAPLCapabilityAgent::sendUserEvent(
    const aplCapabilityCommonInterfaces::aplEventPayload::UserEvent& eventPayload) {
    m_executor->submit([this, eventPayload]() {
        auto token = eventPayload.token;
        ACSDK_DEBUG5(LX("executeOnSendEvent").d("token", token));
        rapidjson::Document payload(rapidjson::kObjectType);
        auto& alloc = payload.GetAllocator();

        payload.AddMember(rapidjson::StringRef(PRESENTATION_TOKEN), rapidjson::Value(token.c_str(), alloc), alloc);

        rapidjson::Document argsDoc(rapidjson::kArrayType, &alloc);
        if (argsDoc.Parse(eventPayload.arguments).HasParseError()) {
            ACSDK_ERROR(LX("executeOnSendEventFailed").d("reason", "Failed to parse arguments document"));
            return;
        }

        payload.AddMember(ARGUMENTS_KEY, argsDoc.Move(), alloc);

        rapidjson::Document sourceDoc(rapidjson::kObjectType, &alloc);
        if (sourceDoc.Parse(eventPayload.source).HasParseError()) {
            ACSDK_ERROR(LX("executeOnSendEventFailed").d("reason", "Failed to parse source document"));
            return;
        }

        payload.AddMember(SOURCE_KEY, sourceDoc.Move(), alloc);

        rapidjson::Document componentsDoc(rapidjson::kObjectType, &alloc);
        if (componentsDoc.Parse(eventPayload.components).HasParseError()) {
            ACSDK_ERROR(LX("executeOnSendEventFailed")
                            .d("reason", "Failed to parse components document, ignoring components field"));
        } else {
            payload.AddMember(COMPONENTS_KEY, componentsDoc.Move(), alloc);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!payload.Accept(writer)) {
            ACSDK_ERROR(LX("executeOnSendEventFailed").d("reason", "Error serializing payload"));
            return;
        }
        executeSendEvent(m_avsNamespace, USER_EVENT, buffer.GetString());
    });
}

void BaseAPLCapabilityAgent::sendDataSourceFetchRequestEvent(
    const aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch& payload) {
    m_executor->submit([this, payload]() {
        auto token = payload.token;
        ACSDK_DEBUG5(LX("executeOnsendDataSourceFetchRequestEvent").d("token", token));
        rapidjson::Document document;
        if (document.Parse(payload.fetchPayload).HasParseError()) {
            ACSDK_ERROR(LX("executeOnsendDataSourceFetchRequestEventFailed").m("Failed to parse document"));
            return;
        }
        document.AddMember(
            rapidjson::StringRef(PRESENTATION_TOKEN),
            rapidjson::Value(token.c_str(), document.GetAllocator()),
            document.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!document.Accept(writer)) {
            ACSDK_ERROR(LX("executeOnsendDataSourceFetchRequestEventFailed").m("Error serializing payload"));
            return;
        }
        auto str = buffer.GetString();

        if (payload.dataSourceType == DYNAMIC_INDEX_LIST) {
            executeSendEvent(m_avsNamespace, LOAD_INDEX_LIST_DATA, str);
        } else if (payload.dataSourceType == DYNAMIC_TOKEN_LIST) {
            executeSendEvent(m_avsNamespace, LOAD_TOKEN_LIST_DATA, str);
        } else {
            ACSDK_WARN(
                LX("sendDataSourceFetchRequestEventIgnored").d("reason", "Trying to process unknown data source."));
            return;
        }
    });
}

void BaseAPLCapabilityAgent::sendRuntimeErrorEvent(
    const aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& payload) {
    m_executor->submit([this, payload]() {
        auto token = payload.token;
        ACSDK_DEBUG5(LX("executeOnsendRuntimeErrorEvent").d("token", token));

        rapidjson::Document document;
        if (document.Parse(payload.errors).HasParseError()) {
            ACSDK_ERROR(LX("executeOnsendRuntimeErrorEventFailed").m("Failed to parse document"));
            return;
        }
        document.AddMember(
            rapidjson::StringRef(PRESENTATION_TOKEN),
            rapidjson::Value(token.c_str(), document.GetAllocator()),
            document.GetAllocator());
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        if (!document.Accept(writer)) {
            ACSDK_ERROR(LX("executeOnsendRuntimeErrorEventFailed").m("Error serializing payload"));
            return;
        }
        auto str = buffer.GetString();
        executeSendEvent(m_avsNamespace, RUNTIME_ERROR, str);
    });
}

void BaseAPLCapabilityAgent::doShutdown() {
    m_proactiveStateTimer.stop();
    m_executor->shutdown();

    executeClearExecuteCommands("BaseAPLCapabilityAgentShuttingDown");
    if (m_visualStateProvider) {
        m_contextManager->removeStateProvider(m_visualContextHeader);
    }
    m_visualStateProvider.reset();
    m_messageSender.reset();
    m_contextManager.reset();
}

void BaseAPLCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    /*
     * Check result too, to catch cases where DirectiveInfo was created locally, without a nullptr result.
     * In those cases there is no messageId to remove because no result was expected.
     */
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void BaseAPLCapabilityAgent::setHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info) {
        if (info->result) {
            info->result->setCompleted();
        } else {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to complete directive", ExceptionErrorType::INTERNAL_ERROR);
        }
        removeDirective(info);
    }
}

void BaseAPLCapabilityAgent::handleRenderDocumentDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));

    m_executor->submit([this, info]() {
        ACSDK_DEBUG9(LX("handleRenderDocumentDirectiveInExecutor").sensitive("payload", info->directive->getPayload()));
        rapidjson::Document payload;
        if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        std::string presentationToken;
        if (!APLPayloadParser::extractPresentationToken(payload, presentationToken)) {
            ACSDK_ERROR(LX("handleRenderDocumentDirectiveFailedInExecutor").d("reason", "NoPresentationToken"));
            sendExceptionEncounteredAndReportFailed(info, "missing presentationToken");
            return;
        }

        auto timeoutTypeStr = APLPayloadParser::extractAPLTimeoutType(payload);
        if (timeoutTypeStr.empty()) {
            ACSDK_ERROR(LX("handleRenderDocumentDirectiveFailedInExecutor").d("reason", "NoTimeoutTypeField"));
        } else {
            // Validate timeoutType
            auto maybeValidTimeout = aplCapabilityCommonInterfaces::convertToTimeoutType(timeoutTypeStr);
            if (!maybeValidTimeout.hasValue()) {
                ACSDK_ERROR(LX("handleRenderDocumentDirectiveFailedInExecutor")
                                .d("reason", "InvalidTimeoutType")
                                .d("receivedTimeoutType", timeoutTypeStr));
                sendExceptionEncounteredAndReportFailed(info, "invalid timeoutType");
                return;
            }
        }

        auto aplDoc = APLPayloadParser::extractDocument(payload);
        if (aplDoc.empty()) {
            ACSDK_ERROR(LX("handleRenderDocumentDirectiveFailedInExecutor").d("reason", "NoDocument"));
            sendExceptionEncounteredAndReportFailed(info, "missing APLdocument");
            return;
        }

        executeRenderDocument(info);
    });
}

void BaseAPLCapabilityAgent::handleShowDocumentDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, info]() {
        rapidjson::Document payload;
        if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }
        auto presentationFieldNames = getPresentationSessionFieldNames();
        auto presentationSession = APLPayloadParser::extractPresentationSession(
            presentationFieldNames.skillId, presentationFieldNames.presentationSessionId, payload);
        if (m_presentationSession.skillId != presentationSession.skillId ||
            m_presentationSession.id != presentationSession.id) {
            ACSDK_ERROR(LX("handleShowDocumentDirectiveFailedInExecutor").d("reason", "NoMatchingPresentationSession"));
            sendExceptionEncounteredAndReportFailed(info, "no matching presentation session");
        }
        notifyObservers(
            [this](std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
                observer->onShowDocument(m_lastRenderedAPLToken);
            });
    });
}

void BaseAPLCapabilityAgent::handleExecuteCommandDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, info]() {
        ACSDK_DEBUG5(LX("handleExecuteCommandDirectiveInExecutor"));
        rapidjson::Document payload;
        if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        std::string presentationToken;
        if (!APLPayloadParser::extractPresentationToken(payload, presentationToken)) {
            ACSDK_ERROR(LX("handleExecuteCommandDirectiveFailedInExecutor").d("reason", "NoPresentationToken"));
            sendExceptionEncounteredAndReportFailed(info, "missing presentationToken");
            return;
        }

        if (!jsonUtils::jsonArrayExists(payload, COMMANDS_FIELD)) {
            ACSDK_ERROR(LX("handleExecuteCommandDirectiveFailedInExecutor")
                            .d("reason", "No command array in the ExecuteCommand directive."));
            sendExceptionEncounteredAndReportFailed(info, "missing commands");
            return;
        }

        if (presentationToken != m_lastRenderedAPLToken) {
            sendExceptionEncounteredAndReportFailed(
                info, "token mismatch between ExecuteCommand and last rendering directive.");
            ACSDK_ERROR(
                LX("handleExecuteCommandDirectiveFailedInExecutor")
                    .d("reason",
                       "presentationToken in executeCommand does not match the one from last displayed directive."));
            return;
        }

        m_lastExecuteCommandTokenAndDirective = std::make_pair(presentationToken, info);
        executeExecuteCommand(info);
    });
}

void BaseAPLCapabilityAgent::handleDynamicListDataDirective(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& sourceType) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor->submit([this, info, sourceType]() {
        ACSDK_DEBUG9(
            LX("handleDynamicListDataDirectiveInExecutor").sensitive("payload", info->directive->getPayload()));
        rapidjson::Document payload;
        if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }
        std::string presentationToken;
        if (!APLPayloadParser::extractPresentationToken(payload, presentationToken)) {
            ACSDK_ERROR(LX("handleDynamicListDataDirectiveFailedInExecutor").d("reason", "NoPresentationToken"));
            sendExceptionEncounteredAndReportFailed(info, "missing presentationToken");
            return;
        }

        if (presentationToken != m_lastRenderedAPLToken) {
            sendExceptionEncounteredAndReportFailed(
                info, "token mismatch between DynamicListData and last rendering directive.");
            ACSDK_ERROR(
                LX("handleDynamicListDataDirectiveFailedInExecutor")
                    .d("reason",
                       "presentationToken in DynamicListData does not match the one from last displayed directive."));
            return;
        }

        // Core will do checks for us for content of it, so just pass through.
        executeDataSourceUpdate(info, sourceType);
    });
}

void BaseAPLCapabilityAgent::handleUnknownDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleUnknownDirective").m("nullDirectiveInfo"));
        sendExceptionEncounteredAndReportFailed(
            info, "nullDirectiveInfo", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return;
    }
    ACSDK_ERROR(LX("requestedToHandleUnknownDirective")
                    .d("reason", "unknownDirective")
                    .d("namespace", info->directive->getNamespace())
                    .d("name", info->directive->getName()));
    m_executor->submit([this, info] {
        const std::string exceptionMessage =
            "unexpected directive " + info->directive->getNamespace() + ":" + info->directive->getName();

        sendExceptionEncounteredAndReportFailed(
            info, exceptionMessage, ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
    });
}

void BaseAPLCapabilityAgent::executeDataSourceUpdate(
    std::shared_ptr<DirectiveInfo> info,
    const std::string& sourceType) {
    ACSDK_DEBUG5(LX(__func__));
    rapidjson::Document payload;
    if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
        sendExceptionEncounteredAndReportFailed(
            info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return;
    }
    std::string presentationToken;
    if (!APLPayloadParser::extractPresentationToken(payload, presentationToken)) {
        ACSDK_ERROR(LX("executeDataSourceUpdateFailed").d("reason", "NoPresentationToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing presentationToken");
        return;
    }
    notifyObservers([sourceType, info, presentationToken](
                        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
        observer->onDataSourceUpdate(sourceType, info->directive->getPayload(), presentationToken);
    });

    setHandlingCompleted(info);
}

void BaseAPLCapabilityAgent::executeRenderDocument(
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    m_lastDisplayedDirective = info;

    if (getDirectiveType(m_lastDisplayedDirective) == DirectiveType::RENDER_DOCUMENT) {
        m_documentRendered = false;
        startMetricsEvent(MetricEvent::RENDER_DOCUMENT);
        rapidjson::Document doc;
        if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &doc)) {
            ACSDK_WARN(LX("executeRenderDocument").m("Error parsing document"));
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            recordRenderComplete(std::chrono::steady_clock::now());
            return;
        }
        std::string newToken;
        APLPayloadParser::extractPresentationToken(doc, newToken);
        auto windowId = APLPayloadParser::extractWindowId(doc);
        ACSDK_DEBUG3(
            LX(__func__).d("previousToken", m_lastRenderedAPLToken).d("newToken", newToken).d("windowId", windowId));
        auto document = APLPayloadParser::extractDocument(doc);
        auto datasources = APLPayloadParser::extractDatasources(doc);
        auto supportedViewports = APLPayloadParser::extractSupportedViewports(doc);
        auto timeoutType =
            aplCapabilityCommonInterfaces::convertToTimeoutType(APLPayloadParser::extractAPLTimeoutType(doc));
        if (!timeoutType.hasValue()) {
            ACSDK_WARN(LX("extractTimeoutTypeFailed").d("reason", "Invalid timeoutType field, using SHORT lifespan"));
        }
        auto presentationFieldNames = getPresentationSessionFieldNames();
        auto presentationSession = APLPayloadParser::extractPresentationSession(
            presentationFieldNames.skillId, presentationFieldNames.presentationSessionId, doc);
        notifyObservers(
            [this, document, datasources, newToken, windowId, timeoutType, supportedViewports, presentationSession](
                std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
                observer->onRenderDocument(
                    document,
                    datasources,
                    newToken,
                    windowId,
                    timeoutType.valueOr(alexaClientSDK::aplCapabilityCommonInterfaces::APLTimeoutType::SHORT),
                    m_avsNamespace,
                    supportedViewports,
                    presentationSession,
                    m_renderReceivedTime,
                    shared_from_this());
            });
        m_renderReceivedTime = {};
        m_lastRenderedAPLToken = newToken;
    } else {
        sendExceptionEncounteredAndReportFailed(m_lastDisplayedDirective, "unknown directive type");
    }
}

void BaseAPLCapabilityAgent::executeExecuteCommand(
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));

    rapidjson::Document payload;
    if (!APLPayloadParser::parseDirectivePayload(info->directive->getPayload(), &payload)) {
        sendExceptionEncounteredAndReportFailed(
            info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return;
    }
    std::string presentationToken;
    if (!APLPayloadParser::extractPresentationToken(payload, presentationToken)) {
        ACSDK_ERROR(LX("executeExecuteCommandFailed").d("reason", "NoPresentationToken"));
        sendExceptionEncounteredAndReportFailed(info, "missing presentationToken");
        return;
    }
    notifyObservers([presentationToken, info](
                        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer) {
        observer->onExecuteCommands(info->directive->getPayload(), presentationToken);
    });
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> BaseAPLCapabilityAgent::
    getCapabilityConfigurations() {
    return getAPLCapabilityConfigurations(m_APLVersion);
}

void BaseAPLCapabilityAgent::executeSendEvent(
    const std::string& avsNamespace,
    const std::string& name,
    const std::string& payload) {
    std::string payload_copy;
    if (shouldPackPresentationSessionToAvsEvents()) {
        rapidjson::Document doc;
        doc.Parse(payload);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        addPresentationSessionPayload(&doc);
        doc.Accept(writer);
        payload_copy = buffer.GetString();
    } else {
        payload_copy = payload;
    }
    m_events.push(std::make_tuple(avsNamespace, name, payload_copy));
    m_contextManager->getContext(shared_from_this());
}

void BaseAPLCapabilityAgent::onContextAvailable(const std::string& jsonContext) {
    auto task = [this, jsonContext]() {
        ACSDK_DEBUG9(LX("onContextAvailableExecutor"));

        if (!m_events.empty()) {
            auto event = m_events.front();

            auto msgIdAndJsonEvent = avsCommon::avs::buildJsonEventString(
                std::get<0>(event), std::get<1>(event), "", std::get<2>(event), jsonContext);
            auto userEventMessage = std::make_shared<avsCommon::avs::MessageRequest>(msgIdAndJsonEvent.second);
            ACSDK_DEBUG9(LX("Sending event to AVS").d("namespace", std::get<0>(event)).d("name", std::get<1>(event)));
            m_messageSender->sendMessage(userEventMessage);

            m_events.pop();
        }
    };

    m_executor->submit(task);
}

void BaseAPLCapabilityAgent::onContextFailure(const ContextRequestError error) {
    ACSDK_ERROR(LX(__func__).d("reason", "contextRequestErrorOccurred").d("error", error));
}

void BaseAPLCapabilityAgent::provideState(
    const avsCommon::avs::NamespaceAndName& stateProviderName,
    unsigned int stateRequestToken) {
    ACSDK_DEBUG3(LX(__func__)
                     .d("namespace", stateProviderName.nameSpace)
                     .d("name", stateProviderName.name)
                     .d("token", stateRequestToken));
    m_executor->submit([this, stateRequestToken]() { executeProvideState(stateRequestToken); });
}

void BaseAPLCapabilityAgent::executeProvideState(unsigned int stateRequestToken) {
    ACSDK_DEBUG3(LX(__func__).d("token", stateRequestToken));

    if (!m_visualStateProvider) {
        ACSDK_ERROR(LX("executeProvideStateFailed").d("reason", "no visualStateProvider"));
        return;
    }

    if (m_lastDisplayedDirective && !m_lastRenderedAPLToken.empty() &&
        getDirectiveType(m_lastDisplayedDirective) == DirectiveType::RENDER_DOCUMENT) {
        m_visualStateProvider->provideState(m_lastRenderedAPLToken, stateRequestToken);
    } else {
        m_contextManager->setState(m_visualContextHeader, "", StateRefreshPolicy::SOMETIMES, stateRequestToken);
        m_lastReportedState.clear();
    }
}

void BaseAPLCapabilityAgent::onVisualContextAvailable(
    avsCommon::sdkInterfaces::ContextRequestToken requestToken,
    const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context) {
    ACSDK_DEBUG3(LX(__func__).d("requestToken", requestToken));
    m_executor->submit([this, requestToken, context]() { executeOnVisualContextAvailable(requestToken, context); });
}

void BaseAPLCapabilityAgent::executeOnVisualContextAvailable(
    const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
    const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context) {
    auto token = context.token;
    ACSDK_DEBUG3(LX("onVisualContextAvailableExecutor"));

    rapidjson::Document doc(rapidjson::kObjectType);
    doc.SetObject();
    auto& allocator = doc.GetAllocator();
    std::string payload;

    doc.AddMember(TOKEN_KEY, rapidjson::Value(token.c_str(), allocator), allocator);

    if (!context.version.empty()) {
        doc.AddMember(VERSION_KEY, rapidjson::Value(context.version.c_str(), allocator), allocator);
    } else {
        doc.AddMember(VERSION_KEY, rapidjson::Value(m_APLVersion.c_str(), allocator), allocator);
    }

    if (!context.visualContext.empty()) {
        rapidjson::Document components(rapidjson::kArrayType, &allocator);
        rapidjson::Document component(rapidjson::kObjectType, &allocator);

        if (component.Parse(context.visualContext).HasParseError()) {
            ACSDK_ERROR(LX("onVisualContextAvailableExecutor").d("reason", "Failed to parse visualContext document"));
        } else {
            /// Add visual context info
            components.PushBack(component, allocator);
            doc.AddMember(VISUAL_CONTEXT_KEY, components.Move(), allocator);
        }
    }

    if (!context.datasourceContext.empty()) {
        rapidjson::Document datasource(rapidjson::kObjectType, &allocator);
        if (datasource.Parse(context.datasourceContext).HasParseError()) {
            ACSDK_ERROR(
                LX("onVisualContextAvailableExecutor").d("reason", "Failed to parse datasourceContext document"));
        } else {
            /// Add datasource context info
            doc.AddMember(DATASOURCE_CONTEXT_KEY, datasource.Move(), allocator);
        }
    }
    //  add presentationSession to payload
    addPresentationSessionPayload(&doc);
    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    payload = buffer.GetString();

    CapabilityState state(payload);
    m_lastReportTime = std::chrono::steady_clock::now();
    m_stateReportPending = false;
    if (PROACTIVE_STATE_REQUEST_TOKEN == requestToken) {
        // Proactive visualContext report
        if (m_lastReportedState != context.visualContext) {
            m_contextManager->reportStateChange(
                m_visualContextHeader, state, AlexaStateChangeCauseType::ALEXA_INTERACTION);
            m_lastReportedState = context.visualContext;
        }
    } else {
        if (!m_lastRenderedAPLToken.empty()) {
            m_contextManager->provideStateResponse(m_visualContextHeader, state, requestToken);
        } else {
            // Since requesting the visualContext, APL is no longer being displayed
            // Set presentationSession as the state.
            m_contextManager->setState(
                m_visualContextHeader, getPresentationSessionPayload(), StateRefreshPolicy::SOMETIMES, requestToken);
            m_lastReportedState.clear();
        }
    }
}

void BaseAPLCapabilityAgent::processRenderDocumentResult(
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    const bool result,
    const std::string& error) {
    m_executor->submit([this, token, result, error]() {
        if (token.empty()) {
            ACSDK_WARN(LX("processRenderDocumentResultFailedInExecutor").d("reason", "token is empty"));
            return;
        }

        if (!m_lastDisplayedDirective) {
            ACSDK_WARN(LX("processRenderDocumentResultFailedInExecutor").d("reason", "no outstanding directive"));
            return;
        }

        ACSDK_DEBUG3(LX("processRenderDocumentResultExecutor").d("token", token).d("result", result));

        // Clear state if rendering has failed and this is the active APL token Note: in most cases
        // m_lastRenderedAPLToken will be the same as directiveToken but this is not guaranteed
        if (!result && token == m_lastRenderedAPLToken) {
            m_presentationSession = {};
            m_lastRenderedAPLToken.clear();
        }
        rapidjson::Document doc;
        if (!APLPayloadParser::parseDirectivePayload(m_lastDisplayedDirective->directive->getPayload(), &doc)) {
            ACSDK_ERROR(LX("processRenderDocumentResultFailedInExecutor").m("Error parsing last displayed directive"));
            return;
        }
        std::string directiveToken;
        if (!APLPayloadParser::extractPresentationToken(doc, directiveToken)) {
            ACSDK_ERROR(LX("processRenderDocumentResultFailedInExecutor").d("reason", "NoValidToken"));
            return;
        }

        if (directiveToken != token) {
            ACSDK_ERROR(LX("processRenderDocumentResultFailedInExecutor")
                            .d("reason", "tokenMismatch")
                            .d("expected", directiveToken)
                            .d("actual", token));
            return;
        }

        if (result) {
            setHandlingCompleted(m_lastDisplayedDirective);
            executeProactiveStateReport();
        } else {
            sendExceptionEncounteredAndReportFailed(m_lastDisplayedDirective, "Renderer failed: " + error);
            resetMetricsEvent(MetricEvent::RENDER_DOCUMENT);
            endMetricsEvent(
                MetricEvent::RENDER_DOCUMENT,
                MetricActivity::ACTIVITY_RENDER_DOCUMENT_FAIL,
                std::chrono::steady_clock::now());
        }
    });
}

void BaseAPLCapabilityAgent::processExecuteCommandsResult(
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
    const std::string& error) {
    m_executor->submit([this, token, event, error]() {
        ACSDK_DEBUG3(LX("processExecuteCommandsResultExecutor")
                         .d("token", token)
                         .d("event", commandExecutionEventToString(event)));

        auto isFailed = aplCapabilityCommonInterfaces::APLCommandExecutionEvent::FAILED == event;
        std::string failureMessage;
        if (isFailed) {
            failureMessage = error;
        } else {
            // If clients had no failures, ensure that this was still a valid directive
            if (token.empty()) {
                isFailed = true;
                failureMessage = "token is empty";
            } else if (token != m_lastExecuteCommandTokenAndDirective.first) {
                isFailed = true;
                failureMessage = "asked to process missing directive";
            } else if (!m_lastExecuteCommandTokenAndDirective.second) {
                isFailed = true;
                failureMessage = "directive to handle is null";
            }
        }
        if (isFailed) {
            ACSDK_ERROR(LX("processExecuteCommandsResultExecutorFailed").d("token", token).d("reason", failureMessage));
            sendExceptionEncounteredAndReportFailed(
                m_lastExecuteCommandTokenAndDirective.second, "Commands execution failed: " + failureMessage);
        } else {
            setHandlingCompleted(m_lastExecuteCommandTokenAndDirective.second);
        }

        m_lastExecuteCommandTokenAndDirective.first.clear();
        executeProactiveStateReport();
    });
}

void BaseAPLCapabilityAgent::startMetricsEvent(MetricEvent metricEvent) {
    switch (metricEvent) {
        case MetricEvent::RENDER_DOCUMENT:
            m_currentActiveTimePoints[metricEvent] = std::chrono::steady_clock::now();
            break;  // Timer Metric events
    }
}

void BaseAPLCapabilityAgent::resetMetricsEvent(MetricEvent metricEvent) {
    switch (metricEvent) {
        case MetricEvent::RENDER_DOCUMENT: {
            m_currentActiveTimePoints.erase(metricEvent);
            break;  // Timer Metric events
        }
    }
}

void BaseAPLCapabilityAgent::endMetricsEvent(
    MetricEvent metricEvent,
    MetricActivity activity,
    const std::chrono::steady_clock::time_point& timestamp) {
    switch (metricEvent) {
        case MetricEvent::RENDER_DOCUMENT: {
            auto eventBuilder =
                alexaClientSDK::avsCommon::utils::metrics::MetricEventBuilder{}
                    .setPriority(alexaClientSDK::avsCommon::utils::metrics::Priority::HIGH)
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                      .setName("APL_TOKEN")
                                      .setValue(m_lastRenderedAPLToken.c_str())
                                      .build())
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                      .setName("SKILL_ID")
                                      .setValue(m_presentationSession.skillId.c_str())
                                      .build())
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                      .setName("DIRECTIVE_MESSAGE_ID")
                                      .setValue(m_lastDisplayedDirective->directive->getMessageId())
                                      .build())
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointStringBuilder{}
                                      .setName("DIALOG_REQUEST_ID")
                                      .setValue(m_lastDisplayedDirective->directive->getDialogRequestId())
                                      .build());

            auto startName = getMetricDataPointName(metricEvent) + ".Start";
            auto startTP = std::chrono::duration_cast<std::chrono::milliseconds>(
                m_currentActiveTimePoints.find(metricEvent) != m_currentActiveTimePoints.end()
                    ? m_currentActiveTimePoints[metricEvent].time_since_epoch()
                    : std::chrono::milliseconds(0));
            auto startEvent =
                eventBuilder.setActivityName(getMetricActivityName(activity) + "-" + startName)
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointDurationBuilder(startTP)
                                      .setName(startName)
                                      .build())
                    .build();

            auto durationName = getMetricDataPointName(metricEvent) + ".TimeTaken";
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                m_currentActiveTimePoints.find(metricEvent) != m_currentActiveTimePoints.end()
                    ? timestamp - m_currentActiveTimePoints[metricEvent]
                    : std::chrono::milliseconds(0));
            auto durationEvent =
                eventBuilder.setActivityName(getMetricActivityName(activity) + "-" + durationName)
                    .addDataPoint(alexaClientSDK::avsCommon::utils::metrics::DataPointDurationBuilder(duration)
                                      .setName(durationName)
                                      .build())
                    .build();

            m_currentActiveTimePoints.erase(metricEvent);
            if (m_metricRecorder != nullptr) {
                ACSDK_DEBUG1(LX(__func__).d("recording metric", getMetricDataPointName(metricEvent)));
                std::lock_guard<std::mutex> lock{m_MetricsRecorderMutex};
                m_metricRecorder->recordMetric(startEvent);
                m_metricRecorder->recordMetric(durationEvent);
            }
            break;
        }
    }
}

void BaseAPLCapabilityAgent::recordRenderComplete(const std::chrono::steady_clock::time_point& timestamp) {
    ACSDK_DEBUG5(LX(__func__));
    m_documentRendered = true;
    /* Document was rendered */
    endMetricsEvent(MetricEvent::RENDER_DOCUMENT, MetricActivity::ACTIVITY_RENDER_DOCUMENT, timestamp);
}

void BaseAPLCapabilityAgent::executeProactiveStateReport() {
    if (m_stateReportCheckInterval.count() == 0 || !m_lastDisplayedDirective || m_lastRenderedAPLToken.empty() ||
        getDirectiveType(m_lastDisplayedDirective) != DirectiveType::RENDER_DOCUMENT || !m_documentRendered) {
        // Not rendering APL or reporting disabled, do not request a state report
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = now - m_lastReportTime;

    if (!m_stateReportPending && duration.count() > m_minStateReportInterval.count()) {
        m_stateReportPending = true;
        m_visualStateProvider->provideState(m_lastRenderedAPLToken, PROACTIVE_STATE_REQUEST_TOKEN);
    }
}

void BaseAPLCapabilityAgent::proactiveStateReport() {
    m_executor->submit([this] { executeProactiveStateReport(); });
}

void BaseAPLCapabilityAgent::onActiveDocumentChanged(
    const aplCapabilityCommonInterfaces::PresentationToken& token,
    const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& session) {
    m_executor->submit([this, token, session] {
        ACSDK_DEBUG5(LX("onActiveDocumentChanged").d("token", token));

        m_lastRenderedAPLToken = token;
        m_presentationSession = session;

        executeProactiveStateReport();
    });
}

void BaseAPLCapabilityAgent::addPresentationSessionPayload(rapidjson::Document* document) {
    const auto presentationSessionKeys = getPresentationSessionFieldNames();
    rapidjson::Document::AllocatorType& allocator = document->GetAllocator();
    rapidjson::Document presentationSession(rapidjson::kObjectType, &allocator);
    auto skillIdFieldValue = rapidjson::Value(presentationSessionKeys.skillId.c_str(), allocator);
    auto presentationSessionIdFieldValue =
        rapidjson::Value(presentationSessionKeys.presentationSessionId.c_str(), allocator);
    presentationSession.AddMember(skillIdFieldValue, m_presentationSession.skillId, allocator);
    presentationSession.AddMember(presentationSessionIdFieldValue, m_presentationSession.id, allocator);

    rapidjson::Document grantedExtensionsDoc(rapidjson::kArrayType, &allocator);
    grantedExtensionsDoc.SetArray();
    for (auto grantedExtension : m_presentationSession.grantedExtensions) {
        rapidjson::Document doc(rapidjson::kObjectType, &allocator);
        doc.AddMember(PRESENTATION_SESSION_URI, grantedExtension.uri, allocator);
        grantedExtensionsDoc.PushBack(doc, allocator);
    }
    presentationSession.AddMember(PRESENTATION_SESSION_GRANTEDEXTENSIONS, grantedExtensionsDoc, allocator);

    rapidjson::Document autoInitializedExtensionsDoc(rapidjson::kArrayType, &allocator);
    autoInitializedExtensionsDoc.SetArray();
    for (auto autoInitializedExtension : m_presentationSession.autoInitializedExtensions) {
        rapidjson::Document doc(rapidjson::kObjectType, &allocator);
        doc.AddMember(PRESENTATION_SESSION_URI, autoInitializedExtension.uri, allocator);
        doc.AddMember(PRESENTATION_SESSION_SETTINGS, autoInitializedExtension.settings, allocator);
        autoInitializedExtensionsDoc.PushBack(doc, allocator);
    }
    presentationSession.AddMember(
        PRESENTATION_SESSION_AUTOINITIALIZEDEXTENSIONS, autoInitializedExtensionsDoc, allocator);

    document->AddMember(rapidjson::StringRef(PRESENTATION_SESSION_FIELD), presentationSession, allocator);
}

std::string BaseAPLCapabilityAgent::getPresentationSessionPayload() {
    rapidjson::Document doc(rapidjson::kObjectType);
    addPresentationSessionPayload(&doc);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    if (!doc.Accept(writer)) {
        ACSDK_ERROR(LX("getPresentationSessionPayloadFailed").d("reason", "error in serializing payload"));
        return "{}";
    }
    return buffer.GetString();
}

}  // namespace aplCapabilityCommon
}  // namespace alexaClientSDK
