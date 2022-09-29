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

#include <utility>
#include <vector>
#include <algorithm>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>

#include "IPCServerSampleApp/GUI/GUIClient.h"
#include "IPCServerSampleApp/IPC/Namespaces/AlexaCaptionsNamespace.h"
#ifdef ENABLE_COMMS
#include "IPCServerSampleApp/IPC/Namespaces/CommunicationsNamespace.h"
#endif
#ifdef ENABLE_RTCSC
#include "IPCServerSampleApp/IPC/Namespaces/LiveViewCameraNamespace.h"
#endif
#include "IPCServerSampleApp/IPC/Namespaces/TemplateRuntimeNamespace.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

static const std::string TAG{"GUIClient"};
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/// The message name for answering a call.
static const std::string NAME_ACCEPT_CALL("acceptCall");

/// The message name for hanging up a call.
static const std::string NAME_STOP_CALL("stopCall");

/// The message name for enabling local video during a call.
static const std::string NAME_ENABLE_LOCAL_VIDEO("enableLocalVideo");

/// The message name for disabling local video during a call.
static const std::string NAME_DISABLE_LOCAL_VIDEO("disableLocalVideo");

/// The message name for sending DTMF keys during a PSTN call.
static const std::string NAME_SEND_DTMF("sendDtmf");

/// The token json key in the message.
static const std::string TOKEN_TAG("token");

/// The enabled json key in the message.
static const std::string ENABLED_TAG("enabled");

/// The DTMF tone json key in the message.
static const std::string DTMF_TONE_TAG("dtmfTone");

/// One second Autorelease timeout
static const std::chrono::seconds AUTORELEASE_DURATION{1};

/// Identifier for the presentation object sent in an APL directive
static const std::string PRESENTATION_TOKEN = "presentationToken";

/// Invalid window id runtime error errors key
static const std::string ERRORS_KEY{"errors"};

/// Invalid window id runtime error type key
static const std::string TYPE_KEY{"type"};

/// Invalid window id runtime error reason key
static const std::string REASON_KEY{"reason"};

/// Invalid window id runtime error list id key
static const std::string LIST_ID_KEY{"listId"};

/// Invalid window id runtime error message key
static const std::string MESSAGE_KEY{"message"};

/// The apl renderer instances json key in the message
static const std::string APL_RENDERER_INSTANCES_TAG{"rendererInstances"};

/// The window id json key in the message.
static const std::string WINDOW_ID_TAG{"windowId"};

/// The json key for the supported extension from window configuration.
static const std::string SUPPORTED_EXTN_KEY{"supportedExtensions"};

/// The payload json key in the message.
static const std::string PAYLOAD_TAG{"payload"};

/// Invalid window id runtime error reason
static const std::string INVALID_OPERATION{"INVALID_OPERATION"};

/// Invalid window id runtime error reason
static const std::string INVALID_WINDOW_ID{"Invalid window id"};

#ifdef ENABLE_COMMS
/// Mapping of DTMF enum to characters for Comms dial tones
static const std::map<std::string, avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone>
    DTMF_TONE_STRING_TO_ENUM_MAP = {
        {"0", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_ZERO},
        {"1", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_ONE},
        {"2", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_TWO},
        {"3", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_THREE},
        {"4", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_FOUR},
        {"5", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_FIVE},
        {"6", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_SIX},
        {"7", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_SEVEN},
        {"8", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_EIGHT},
        {"9", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_NINE},
        {"*", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_STAR},
        {"#", avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone::DTMF_POUND},
};
#endif

using namespace avsCommon::utils::json;
using namespace avsCommon::utils::timing;
using namespace avsCommon::sdkInterfaces::storage;

std::shared_ptr<GUIClient> GUIClient::create(
    std::shared_ptr<MessagingServerInterface> serverImplementation,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface> customerDataManager,
    const std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager,
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo) {
    if (!serverImplementation) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullServerImplementation"));
        return nullptr;
    }

    if (!customerDataManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCustomerDataManager"));
        return nullptr;
    }

    if (!ipcVersionManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullIPCVersionManager"));
        return nullptr;
    }

    return std::shared_ptr<GUIClient>(
        new GUIClient(serverImplementation, customerDataManager, ipcVersionManager, deviceInfo));
}

GUIClient::GUIClient(
    std::shared_ptr<MessagingServerInterface> serverImplementation,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager,
    std::shared_ptr<ipc::IPCVersionManager> ipcVersionManager,
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo) :
        RequiresShutdown{TAG},
        CustomerDataHandler{customerDataManager},
        m_serverImplementation{std::move(serverImplementation)},
        m_hasServerStarted{false},
        m_initMessageReceived{false},
        m_errorState{false},
        m_shouldRestart{false},
        m_ipcVersionManager{std::move(ipcVersionManager)},
        m_limitedInteraction{false},
        m_ipcAPLAgent{nullptr},
        m_deviceInfo{deviceInfo} {
    m_messageHandlers.emplace(
        NAME_ACCEPT_CALL, [this](rapidjson::Document& payload) { executeHandleAcceptCall(payload); });
    m_messageHandlers.emplace(NAME_STOP_CALL, [this](rapidjson::Document& payload) { executeHandleStopCall(payload); });

    m_messageHandlers.emplace(
        NAME_ENABLE_LOCAL_VIDEO, [this](rapidjson::Document& payload) { executeHandleEnableLocalVideo(payload); });
    m_messageHandlers.emplace(
        NAME_DISABLE_LOCAL_VIDEO, [this](rapidjson::Document& payload) { executeHandleDisableLocalVideo(payload); });

#ifdef ENABLE_COMMS
    m_messageHandlers.emplace(NAME_SEND_DTMF, [this](rapidjson::Document& payload) { executeHandleSendDtmf(payload); });
#endif

    registerNamespaces();
}

void GUIClient::doShutdown() {
    ACSDK_DEBUG3(LX(__func__));
    stop();
    m_executor.shutdown();
    m_guiManager.reset();
    m_aplClientBridge.reset();
    m_messageListener.reset();
    m_observer.reset();
    m_serverImplementation.reset();

    std::lock_guard<std::mutex> lock{m_mapMutex};
    m_focusObservers.clear();
}

void GUIClient::setGUIManager(std::shared_ptr<gui::GUIServerInterface> guiManager) {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this, guiManager]() {
        if (!m_aplClientBridge) {
            ACSDK_ERROR(LX("setGUIManagerFailed").d("reason", "nullAplRenderer"));
            return;
        }
        m_guiManager = guiManager;
        m_aplClientBridge->setGUIManager(guiManager);
    });
}

void GUIClient::registerNamespaces() {
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_INTERACTION_MANAGER,
        ipc::IPCNamespaces::IPC_MSG_VERSION_INTERACTION_MANAGER);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_APL, ipc::IPCNamespaces::IPC_MSG_VERSION_APL);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_SYSTEM, ipc::IPCNamespaces::IPC_MSG_VERSION_SYSTEM);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_AUDIO_FOCUS_MANAGER,
        ipc::IPCNamespaces::IPC_MSG_VERSION_AUDIO_FOCUS_MANAGER);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_LOGGER, ipc::IPCNamespaces::IPC_MSG_VERSION_LOGGER);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_SESSION_SETUP, ipc::IPCNamespaces::IPC_MSG_VERSION_SESSION_SETUP);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_DO_NOT_DISTURB, ipc::IPCNamespaces::IPC_MSG_VERSION_DO_NOT_DISTURB);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_TEMPLATE_RUNTIME, ipc::IPCNamespaces::IPC_MSG_VERSION_TEMPLATE_RUNTIME);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_WINDOW_MANAGER, ipc::IPCNamespaces::IPC_MSG_VERSION_WINDOW_MANAGER);
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_CAPTIONS, ipc::IPCNamespaces::IPC_MSG_VERSION_CAPTIONS);
#ifdef ENABLE_RTCSC
    m_ipcVersionManager->registerNamespaceVersionEntry(
        ipc::IPCNamespaces::IPC_MSG_NAMESPACE_LIVE_VIEW_CAMERA, ipc::IPCNamespaces::IPC_MSG_VERSION_LIVE_VIEW_CAMERA);
#endif
#ifdef ENABLE_COMMS
    m_ipcVersionManager->registerNamespaceVersionEntry(
        IPC_MSG_NAMESPACE_COMMUNICATIONS, IPC_MSG_VERSION_COMMUNICATIONS);
#endif
}

void GUIClient::setAplClientBridge(std::shared_ptr<AplClientBridge> aplClientBridge, bool aplVersionChanged) {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this, aplClientBridge, aplVersionChanged]() {
        m_aplClientBridge = aplClientBridge;
        if (aplVersionChanged) {
            m_shouldRestart = true;
        }
    });
}

bool GUIClient::acquireFocus(
    std::string avsInterface,
    std::string channelName,
    avsCommon::avs::ContentType contentType,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    ACSDK_DEBUG5(LX(__func__));

    return m_executor
        .submit([this, avsInterface, channelName, contentType, channelObserver]() {
            return executeAcquireFocus(avsInterface, channelName, contentType, channelObserver);
        })
        .get();
}

bool GUIClient::releaseFocus(
    std::string avsInterface,
    std::string channelName,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    ACSDK_DEBUG5(LX(__func__));
    return m_executor
        .submit([this, avsInterface, channelName, channelObserver]() {
            return executeReleaseFocus(avsInterface, channelName, channelObserver);
        })
        .get();
}

#ifdef ENABLE_COMMS
void GUIClient::sendCallStateInfo(
    const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, callStateInfo]() { executeSendCallStateInfo(callStateInfo); });
}
#endif

bool GUIClient::executeAcquireFocus(
    std::string avsInterface,
    std::string channelName,
    avsCommon::avs::ContentType contentType,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    return m_guiManager->handleFocusAcquireRequest(avsInterface, channelName, contentType, channelObserver);
}

bool GUIClient::executeReleaseFocus(
    std::string avsInterface,
    std::string channelName,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) {
    return m_guiManager->handleFocusReleaseRequest(avsInterface, channelName, channelObserver);
}

bool GUIClient::isReady() {
    return m_hasServerStarted && m_initMessageReceived && !m_errorState;
}

void GUIClient::setMessageListener(std::shared_ptr<MessageListenerInterface> messageListener) {
    m_executor.submit([this, messageListener]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messageListener = messageListener;
    });
}

bool GUIClient::start() {
    m_executor.submit([this]() {
        // start the server asynchronously.
        m_serverThread = std::thread(&GUIClient::serverThread, this);
    });

    return true;
};

void GUIClient::serverThread() {
    ACSDK_DEBUG3(LX("serverThread"));
    if (m_serverImplementation) {
        m_serverImplementation->setMessageListener(shared_from_this());
        m_serverImplementation->setObserver(shared_from_this());

        m_hasServerStarted = true;

        if (!m_serverImplementation->start()) {
            m_hasServerStarted = false;
            m_errorState = true;
            ACSDK_ERROR(LX("serverThreadFailed").d("reason", "start failed"));
            return;
        }
    } else {
        ACSDK_ERROR(LX("serverThreadFailed").d("reason", "noServerImplementation"));
    }
}

void GUIClient::stop() {
    ACSDK_DEBUG3(LX(__func__));
    m_executor.submit([this]() {
        if (m_hasServerStarted) {
            m_serverImplementation->stop();
        }
        m_hasServerStarted = m_initMessageReceived = m_errorState = false;
    });
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

void GUIClient::onMessage(const std::string& jsonMessage) {
    m_executor.submit([this, jsonMessage]() { m_ipcRouter->onMessage(std::move(jsonMessage)); });
}

void GUIClient::executeHandleAcceptCall(rapidjson::Document& payload) {
    m_guiManager->acceptCall();
}

void GUIClient::executeHandleStopCall(rapidjson::Document& payload) {
    m_guiManager->stopCall();
}

void GUIClient::executeHandleEnableLocalVideo(rapidjson::Document& payload) {
    m_guiManager->enableLocalVideo();
}

void GUIClient::executeHandleDisableLocalVideo(rapidjson::Document& payload) {
    m_guiManager->disableLocalVideo();
}

#ifdef ENABLE_COMMS
void GUIClient::executeHandleSendDtmf(rapidjson::Document& payload) {
    std::string dtmfString;
    if (!jsonUtils::retrieveValue(payload, DTMF_TONE_TAG, &dtmfString)) {
        ACSDK_ERROR(LX("handleSendDtmfRequestFailed").d("reason", "dtmfToneNotFound"));
        return;
    }
    ACSDK_DEBUG3(LX("handleSendDtmfRequest").d(DTMF_TONE_TAG, dtmfString));

    auto dtmfIterator = DTMF_TONE_STRING_TO_ENUM_MAP.find(dtmfString);
    if (dtmfIterator == DTMF_TONE_STRING_TO_ENUM_MAP.end()) {
        ACSDK_ERROR(LX("handleSendDtmfRequestFailed").d("unknown dtmfTone", dtmfString));
        return;
    }
    avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone = dtmfIterator->second;
    m_guiManager->sendDtmf(dtmfTone);
}
#endif

void GUIClient::focusAcquireRequest(
    const GUIToken token,
    const std::string& avsInterface,
    const std::string& channelName,
    avsCommon::avs::ContentType contentType) {
    bool result = true;
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> focusObserver;
    {
        std::lock_guard<std::mutex> lock{m_mapMutex};
        if (m_focusObservers.count(token) == 0) {
            m_focusObservers[token] =
                std::make_shared<ProxyFocusObserver>(avsInterface, token, shared_from_this(), channelName);
            focusObserver = m_focusObservers[token];
        } else {
            result = false;
        }
    }

    if (!result) {
        ACSDK_ERROR(LX("focusAcquireRequestFail").d("token", token).d("reason", "observer already exists"));
        executeSendFocusResponse(token, false);
        return;
    }

    result = executeAcquireFocus(avsInterface, channelName, contentType, focusObserver);
    if (!result) {
        ACSDK_ERROR(LX("focusAcquireRequestFail").d("token", token).d("reason", "acquireChannel returned false"));
        executeSendFocusResponse(token, false);
        return;
    }

    executeSendFocusResponse(token, true);
}

void GUIClient::focusReleaseRequest(
    const GUIToken token,
    const std::string& avsInterface,
    const std::string& channelName) {
    bool result = true;

    std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> focusObserver;
    {
        std::lock_guard<std::mutex> lock{m_mapMutex};
        auto it = m_focusObservers.find(token);
        if (it == m_focusObservers.end()) {
            result = false;
        } else {
            focusObserver = it->second;
        }
    }

    if (!result || !focusObserver) {
        ACSDK_ERROR(LX("focusReleaseRequestFail").d("token", token).d("reason", "no observer found"));
        executeSendFocusResponse(token, false);
        return;
    }

    result = executeReleaseFocus(avsInterface, channelName, focusObserver);
    if (!result) {
        ACSDK_ERROR(LX("focusReleaseRequestFail").d("token", token).d("reason", "releaseChannel returned false"));
        executeSendFocusResponse(token, false);
        return;
    }
    executeSendFocusResponse(token, true);
}

void GUIClient::executeSendFocusResponse(const GUIToken token, const bool result) {
    m_audioFocusManagerIPCHandler->processChannelResult(token, result);
}

void GUIClient::handleOnFocusChangedReceivedConfirmation(GUIToken token) {
    std::lock_guard<std::mutex> lock{m_mapMutex};
    auto currentAutoReleaseTimer = m_autoReleaseTimers.find(token);
    if (currentAutoReleaseTimer != m_autoReleaseTimers.end()) {
        if (!currentAutoReleaseTimer->second) {
            ACSDK_ERROR(LX("processOnFocusChangedReceivedConfirmationFail")
                            .d("token", token)
                            .d("reason", "autoReleaseTimer is null"));
            return;
        }
        currentAutoReleaseTimer->second->stop();
    }
}

void GUIClient::handleLocalRenderDocument(rapidjson::Document& payload) {
    std::string token;
    if (!jsonUtils::retrieveValue(payload, TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("handleLocalRenderDocumentFailed").d("reason", "tokenNotFound"));
        return;
    }

    std::string documentPayload;
    if (!jsonUtils::retrieveValue(payload, PAYLOAD_TAG, &documentPayload)) {
        ACSDK_ERROR(LX("handleLocalRenderDocumentFailed").d("reason", "payloadNotFound"));
        return;
    }

    std::string windowId;
    if (!jsonUtils::retrieveValue(payload, WINDOW_ID_TAG, &windowId)) {
        ACSDK_ERROR(LX("handleLocalRenderDocumentFailed").d("reason", "windowIdNotFound"));
        return;
    }

    if (!m_ipcAPLAgent) {
        m_ipcAPLAgent = IPCAPLAgent::create(shared_from_this());
        m_ipcAPLAgent->setAPLMaxVersion(m_aplRuntimePresentationAdapter->getAPLRuntimeVersion());
    }

    if (m_aplRuntimePresentationAdapter) {
        m_aplRuntimePresentationAdapter->renderDocumentWithoutPresentation(
            documentPayload, token, windowId, std::chrono::steady_clock::now(), TAG, m_ipcAPLAgent);
    } else {
        ACSDK_ERROR(LX("handleLocalRenderDocumentFailed").d("reason", "aplRuntimeAdapterForLocalDocumentNotFound"));
    }
}

void GUIClient::handleLocalClearDocument(rapidjson::Document& payload) {
    std::string token;
    if (!jsonUtils::retrieveValue(payload, TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("handleLocalClearDocumentFailed").d("reason", "tokenNotFound"));
        return;
    }

    if (m_aplRuntimePresentationAdapter) {
        m_aplRuntimePresentationAdapter->clearDocument(token);
    } else {
        ACSDK_ERROR(LX("handleLocalClearDocumentFailed").d("reason", "aplRuntimeAdapterForLocalDocumentNotFound"));
    }
}

void GUIClient::handleLocalExecuteCommands(rapidjson::Document& payload) {
    std::string token;
    if (!jsonUtils::retrieveValue(payload, TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("handleLocalExecuteCommandsFailed").d("reason", "tokenNotFound"));
        return;
    }

    std::string commandsPayload;
    if (!jsonUtils::retrieveValue(payload, PAYLOAD_TAG, &commandsPayload)) {
        ACSDK_ERROR(LX("handleLocalExecuteCommandsFailed").d("reason", "payloadNotFound"));
        return;
    }

    if (m_aplRuntimePresentationAdapter) {
        m_aplRuntimePresentationAdapter->onExecuteCommands(commandsPayload, token);
    } else {
        ACSDK_ERROR(LX("handleLocalExecuteCommandsFailed").d("reason", "aplRuntimeAdapterForLocalDocumentNotFound"));
    }
}
void GUIClient::handleAplEvent(rapidjson::Document& payload) {
    if (!m_aplClientBridge) {
        ACSDK_ERROR(LX("handleAplEventFailed").d("reason", "APL Renderer has not been configured"));
        return;
    }

    std::string eventPayload;
    if (!jsonUtils::retrieveValue(payload, PAYLOAD_TAG, &eventPayload)) {
        ACSDK_ERROR(LX("handleAplEventFailed").d("reason", "payloadNotFound"));
        return;
    }

    std::string windowId;
    if (!jsonUtils::retrieveValue(payload, WINDOW_ID_TAG, &windowId)) {
        ACSDK_ERROR(LX("handleAplEventFailed").d("reason", "windowIdNotFound"));
        return;
    }

    m_aplClientBridge->onMessage(windowId, eventPayload);
}

void GUIClient::handleRenderComplete(rapidjson::Document& payload) {
    std::string windowId;
    if (!jsonUtils::retrieveValue(payload, WINDOW_ID_TAG, &windowId)) {
        ACSDK_ERROR(LX("handleRenderComplete").d("reason", "windowIdNotFound"));
        return;
    }

    m_aplClientBridge->handleRenderingEvent(windowId, APLClient::AplRenderingEvent::DOCUMENT_RENDERED);
}

void GUIClient::handleDisplayMetrics(rapidjson::Document& payload) {
    std::string windowId;
    std::string jsonPayload;

    if (!jsonUtils::retrieveValue(payload, WINDOW_ID_TAG, &windowId)) {
        ACSDK_ERROR(LX("handleDisplayMetricsFailed").d("reason", "windowIdNotFound"));
        return;
    }

    if (!jsonUtils::convertToValue(payload, &jsonPayload)) {
        ACSDK_ERROR(LX("handleDisplayMetricsFailed").d("reason", "unableToConvertPayloadToValue"));
        return;
    }

    m_aplClientBridge->handleDisplayMetrics(windowId, jsonPayload);
}

void GUIClient::setObserver(const std::shared_ptr<MessagingServerObserverInterface>& observer) {
    m_executor.submit([this, observer]() { m_observer = observer; });
}

void GUIClient::onConnectionOpened() {
    ACSDK_DEBUG3(LX("onConnectionOpened"));
    m_executor.submit([this]() {
        if (!m_initThread.joinable()) {
            m_initThread = std::thread(&GUIClient::sendInitRequestAndWait, this);
        } else {
            ACSDK_INFO(LX("onConnectionOpened").m("init thread is not avilable"));
        }

        if (m_observer) {
            m_observer->onConnectionOpened();
        }
        m_guiManager->handleOnMessagingServerConnectionOpened();
    });
}

void GUIClient::onConnectionClosed() {
    ACSDK_DEBUG3(LX("onConnectionClosed"));
    m_executor.submit([this]() {
        if (!m_serverImplementation->isReady()) {
            m_initMessageReceived = false;
        }

        if (m_initThread.joinable()) {
            m_initThread.join();
        }

        if (m_observer) {
            m_observer->onConnectionClosed();
        }
        m_aplClientBridge->onConnectionClosed();
    });
}

void GUIClient::clearData() {
    ACSDK_DEBUG5(LX(__func__));
}

void GUIClient::onLogout() {
    m_shouldRestart = true;
    m_cond.notify_all();
}

SampleAppReturnCode GUIClient::run() {
    ACSDK_DEBUG3(LX("run"));
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock, [this] { return m_shouldRestart || m_errorState; });
    ACSDK_DEBUG3(LX("runExits").d("reason", m_shouldRestart ? "loggedout" : "not initialized"));
    return m_shouldRestart ? SampleAppReturnCode::RESTART
                           : (m_errorState ? SampleAppReturnCode::ERROR : SampleAppReturnCode::OK);
}

void GUIClient::sendInitRequestAndWait() {
    // Wait for server to be ready
    ACSDK_DEBUG9(LX("sendInitRequestAndWait").m("waiting for server to be ready"));
    while (!m_serverImplementation->isReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Send init request message.
    m_guiManager->initClient();

    // Wait for response
    std::unique_lock<std::mutex> lock(m_mutex);
    ACSDK_DEBUG3(LX("start").m("waiting for InitResponse"));
    m_cond.wait(lock, [this] {
        ACSDK_DEBUG9(LX("sendInitRequestAndWait")
                         .d("errorState", m_errorState)
                         .d("initMessage received", m_initMessageReceived));
        return m_errorState || m_initMessageReceived;
    });

    ACSDK_DEBUG3(LX("start").m("InitResponse received"));
    m_aplClientBridge->onConnectionOpened();
}

#ifdef ENABLE_COMMS
void GUIClient::executeSendCallStateInfo(
    const avsCommon::sdkInterfaces::CallStateObserverInterface::CallStateInfo& callStateInfo) {
    auto message = messages::CallStateChangeMessage(callStateInfo);
    sendMessage(message);
}
#endif

bool GUIClient::finalizeClientInitialization(bool errorState) {
    m_errorState = errorState;
    m_initMessageReceived = true;

    m_cond.notify_all();
    if (m_initThread.joinable()) {
        m_initThread.join();
    }

    return true;
}

void GUIClient::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) {
    m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
}

void GUIClient::onCapabilitiesStateChange(
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
    avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& addedOrUpdatedEndpoints,
    const std::vector<avsCommon::sdkInterfaces::endpoints::EndpointIdentifier>& deletedEndpoints) {
    m_limitedInteraction = m_limitedInteraction ||
                           (newState == avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State::FATAL_ERROR);
}

GUIClient::ProxyFocusObserver::ProxyFocusObserver(
    std::string avsInterface,
    const GUIToken token,
    std::shared_ptr<GUIClient> guiClient,
    std::string channelName) :
        m_avsInterface{std::move(avsInterface)},
        m_token{token},
        m_focusBridge{std::move(guiClient)},
        m_channelName{std::move(channelName)} {
}

void GUIClient::ProxyFocusObserver::onFocusChanged(
    avsCommon::avs::FocusState newFocus,
    avsCommon::avs::MixingBehavior behavior) {
    if (newFocus != avsCommon::avs::FocusState::NONE) {
        m_focusBridge->startAutoreleaseTimer(m_avsInterface, m_token, m_channelName);
    }
    m_focusBridge->sendOnFocusChanged(m_token, newFocus);
}

void GUIClient::startAutoreleaseTimer(
    const std::string& avsInterface,
    const GUIToken token,
    const std::string& channelName) {
    std::shared_ptr<avsCommon::utils::timing::Timer> timer = std::make_shared<avsCommon::utils::timing::Timer>();
    {
        std::lock_guard<std::mutex> lock{m_mapMutex};
        m_autoReleaseTimers[token] = timer;
    }

    timer->start(AUTORELEASE_DURATION, [this, avsInterface, token, channelName] {
        autoRelease(avsInterface, token, channelName);
    });
}

void GUIClient::autoRelease(const std::string& avsInterface, const GUIToken token, const std::string& channelName) {
    ACSDK_DEBUG5(LX("autoRelease").d("token", token).d("channelName", channelName));
    m_executor.submit([this, avsInterface, token, channelName]() {
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> focusObserver;
        std::shared_ptr<avsCommon::utils::timing::Timer> autoReleaseTimer;
        {
            std::lock_guard<std::mutex> lock{m_mapMutex};
            focusObserver = m_focusObservers[token];
            if (!focusObserver) {
                ACSDK_CRITICAL(LX("autoReleaseFailed").d("token", token).d("reason", "focusObserver is null"));
                return;
            }
        }
        m_guiManager->handleFocusReleaseRequest(avsInterface, channelName, focusObserver);
    });
}

void GUIClient::sendOnFocusChanged(const GUIToken token, const avsCommon::avs::FocusState state) {
    m_audioFocusManagerIPCHandler->processFocusChanged(token, state);

    if (state == avsCommon::avs::FocusState::NONE) {
        // Remove observer and timer when released.
        std::lock_guard<std::mutex> lock{m_mapMutex};
        if (m_focusObservers.erase(token) == 0) {
            ACSDK_WARN(LX("sendOnFocusChanged").d("reason", "tokenNotFoundWhenRemovingObserver").d("token", token));
        }
        if (m_autoReleaseTimers.erase(token) == 0) {
            ACSDK_WARN(
                LX("sendOnFocusChanged").d("reason", "tokenNotFoundWhenRemovingAutoReleaseTimer").d("token", token));
        }
    }
}

void GUIClient::notifyAuthorizationRequest(const std::string& url, const std::string& code) {
    m_authUrl = url;
    m_authCode = code;
    m_clientId = m_deviceInfo->getClientId();
    sendCompleteAuthorizationMessage(url, code, m_deviceInfo->getClientId());
}

void GUIClient::notifyAuthorizationStateChange(avsCommon::sdkInterfaces::AuthObserverInterface::State state) {
    std::string authState = "UNINITIALIZED";
    switch (state) {
        case avsCommon::sdkInterfaces::AuthObserverInterface::State::UNINITIALIZED:
            authState = "UNINITIALIZED";
            break;
        case avsCommon::sdkInterfaces::AuthObserverInterface::State::REFRESHED:
            authState = "REFRESHED";
            break;
        case avsCommon::sdkInterfaces::AuthObserverInterface::State::EXPIRED:
            authState = "EXPIRED";
            break;
        case avsCommon::sdkInterfaces::AuthObserverInterface::State::UNRECOVERABLE_ERROR:
            authState = "ERROR";
            break;
        case avsCommon::sdkInterfaces::AuthObserverInterface::State::AUTHORIZING:
            authState = "AUTHORIZING";
            break;
    }

    m_authState = authState;
    sendSetAuthorizationStateMessage(authState);
}

void GUIClient::notifyAlexaState(const std::string& state) {
    m_alexaState = state;
    sendSetAlexaStateMessage(state);
}

void GUIClient::setLocales(const std::string& localeStr) {
    m_localeStr = localeStr;
    sendSetLocalesMessage(localeStr);
}

void GUIClient::sendMessage(messages::MessageInterface& message) {
    writeMessage(message.get());
}

void GUIClient::executeSendMessage(messages::MessageInterface& message) {
    executeWriteMessage(message.get());
}

void GUIClient::writeMessage(const std::string& payload) {
    m_executor.submit([this, payload]() { executeWriteMessage(payload); });
}

void GUIClient::executeWriteMessage(const std::string& payload) {
    m_serverImplementation->writeMessage(payload);
}

void GUIClient::handleInitializeAPLRenderers(rapidjson::Document& payload) {
    if (!m_aplClientBridge) {
        ACSDK_ERROR(LX("handleInitializeAPLRenderersFailed").d("reason", "null aplClientBridge"));
        return;
    }

    if (!payload.IsObject()) {
        ACSDK_ERROR(LX("handleInitializeAPLRenderersFailed").d("reason", "payload not an object"));
        return;
    }

    const auto& rendererInstances = payload[APL_RENDERER_INSTANCES_TAG];
    if (!rendererInstances.IsArray()) {
        ACSDK_ERROR(LX("handleInitializeAPLRenderersFailed").d("reason", "window instances not an array"));
        return;
    }

    for (auto const& rendererInstance : rendererInstances.GetArray()) {
        std::string windowId;
        if (!jsonUtils::retrieveValue(rendererInstance, WINDOW_ID_TAG, &windowId)) {
            ACSDK_WARN(LX("handleInitializeAPLRenderersFailed").d("reason", "window ID not found"));
            continue;
        }

        const auto& supportedExtensionsJson = rendererInstance[SUPPORTED_EXTN_KEY];
        if (!supportedExtensionsJson.IsArray()) {
            ACSDK_WARN(LX("handleInitializeAPLRenderersFailed").d("reason", "supportedExtensions is not an array"));
            continue;
        }
        const auto supportedExtensions = jsonUtils::retrieveStringArray<std::set<std::string>>(supportedExtensionsJson);

        ACSDK_DEBUG1(LX(__func__).d("initializingWindow", windowId));
        m_aplClientBridge->initializeRenderer(windowId, supportedExtensions);
    }
}

void GUIClient::reportInvalidWindowIdRuntimeError(const std::string& errorMessage, const std::string& aplToken) {
    rapidjson::Document runtimeErrorPayloadJson(rapidjson::kObjectType);
    auto& alloc = runtimeErrorPayloadJson.GetAllocator();
    rapidjson::Value payload(rapidjson::kObjectType);
    rapidjson::Value errors(rapidjson::kArrayType);
    payload.AddMember(rapidjson::StringRef(PRESENTATION_TOKEN), aplToken, alloc);

    rapidjson::Value error(rapidjson::kObjectType);
    error.AddMember(rapidjson::StringRef(TYPE_KEY), INVALID_OPERATION, alloc);
    error.AddMember(rapidjson::StringRef(REASON_KEY), INVALID_WINDOW_ID, alloc);
    error.AddMember(rapidjson::StringRef(LIST_ID_KEY), "", alloc);
    error.AddMember(rapidjson::StringRef(MESSAGE_KEY), errorMessage, alloc);

    errors.PushBack(error, alloc);
    payload.AddMember(rapidjson::StringRef(ERRORS_KEY), errors, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    if (!payload.Accept(writer)) {
        ACSDK_ERROR(LX("reportInvalidWindowIdRuntimeErrorFailed").d("reason", "Error serializing payload"));
        return;
    }

    // m_guiManager->handleRuntimeErrorEvent(aplToken, sb.GetString());
    ACSDK_WARN(LX("reportInvalidWindowIdRuntimeError").d("reported runtime error", std::string(sb.GetString())));
}

void GUIClient::addToMediaPlayerMap(
    const std::string& name,
    const std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>& mediaPlayer) {
    ACSDK_DEBUG5(LX(__func__));
    m_executor.submit([this, name, mediaPlayer]() { m_mediaPlayerMap[name] = mediaPlayer; });
}

void GUIClient::setAPLRuntimePresentationAdapter(
    const std::shared_ptr<APLRuntimePresentationAdapter>& aplRuntimePresentationAdapter) {
    m_executor.submit(
        [this, aplRuntimePresentationAdapter]() { m_aplRuntimePresentationAdapter = aplRuntimePresentationAdapter; });
}

bool GUIClient::dispatch(const std::string& message) {
    writeMessage(message);

    return true;
}

void GUIClient::initIPCRouter() {
    ACSDK_DEBUG3(LX(__func__));
    m_ipcRouter = ipc::IPCRouter::create(m_serverImplementation, shared_from_this(), m_ipcVersionManager);

    m_audioFocusManagerIPCHandler = ipc::AudioFocusManagerHandler::create(m_ipcRouter, shared_from_this());
    m_systemIPCHandler = ipc::SystemHandler::create(m_ipcRouter, shared_from_this());
}

std::shared_ptr<ipc::IPCRouter> GUIClient::getIPCRouter() {
    return m_ipcRouter;
}

void GUIClient::sendCompleteAuthorizationMessage(
    const std::string& url,
    const std::string& code,
    const std::string& clientId) {
    if (m_systemIPCHandler) {
        m_systemIPCHandler->completeAuthorization(url, code, clientId);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "systemHandlerNotFound"));
    }
}

void GUIClient::sendSetAlexaStateMessage(const std::string& state) {
    if (m_systemIPCHandler) {
        m_systemIPCHandler->setAlexaState(state);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "systemHandlerNotFound"));
    }
}

void GUIClient::sendSetAuthorizationStateMessage(const std::string& authState) {
    if (m_systemIPCHandler) {
        m_systemIPCHandler->setAuthorizationState(authState);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "systemHandlerNotFound"));
    }
}

void GUIClient::sendSetLocalesMessage(const std::string& localeStr) {
    if (m_systemIPCHandler) {
        m_systemIPCHandler->setLocales(localeStr);
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "systemHandlerNotFound"));
    }
}

void GUIClient::acquireChannelRequest(const std::string& message) {
    ACSDK_DEBUG0(LX("acquireChannelRequest"));
    std::string avsInterface;
    if (!jsonUtils::retrieveValue(message, ipc::AVS_INTERFACE_TAG, &avsInterface)) {
        ACSDK_ERROR(LX("acquireChannelRequestFailed").d("reason", "avsInterfaceNotFound"));
        return;
    }

    GUIToken token = 0;
    if (!jsonUtils::retrieveValue(message, ipc::AUDIO_FOCUS_MANAGER_TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("acquireChannelRequestFailed").d("reason", "tokenNotFound"));
        return;
    }

    std::string channelName;
    if (!jsonUtils::retrieveValue(message, ipc::CHANNEL_NAME_TAG, &channelName)) {
        ACSDK_ERROR(LX("acquireChannelRequestFailed").d("reason", "channelNameNotFound"));
        return;
    }

    avsCommon::avs::ContentType contentType = avsCommon::avs::ContentType::UNDEFINED;
    std::string contentTypeStr;
    if (!jsonUtils::retrieveValue(message, ipc::CONTENT_TYPE_TAG, &contentTypeStr)) {
        ACSDK_WARN(LX("acquireChannelRequest").d("reason", "contentTypeUndefined"));
    } else {
        if (contentTypeStr == "MIXABLE") {
            contentType = avsCommon::avs::ContentType::MIXABLE;
        } else if (contentTypeStr == "NONMIXABLE") {
            contentType = avsCommon::avs::ContentType::NONMIXABLE;
        } else {
            ACSDK_WARN(LX("acquireChannelRequest").d("reason", "contentTypeInvalid").d("contentType", contentTypeStr));
        }
    }

    focusAcquireRequest(token, avsInterface, channelName, contentType);
}

void GUIClient::releaseChannelRequest(const std::string& message) {
    std::string avsInterface;
    if (!jsonUtils::retrieveValue(message, ipc::AVS_INTERFACE_TAG, &avsInterface)) {
        ACSDK_ERROR(LX("releaseChannelRequestFaied").d("reason", "avsInterfaceNotFound"));
        return;
    }

    GUIToken token = 0;
    if (!jsonUtils::retrieveValue(message, ipc::AUDIO_FOCUS_MANAGER_TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("releaseChannelRequestFaied").d("reason", "tokenNotFound"));
        return;
    }

    std::string channelName;
    if (!jsonUtils::retrieveValue(message, ipc::CHANNEL_NAME_TAG, &channelName)) {
        ACSDK_ERROR(LX("releaseChannelRequestFaied").d("reason", "channelNameNotFound"));
        return;
    }

    focusReleaseRequest(token, avsInterface, channelName);
}

void GUIClient::focusChangedReport(const std::string& message) {
    GUIToken token = 0;
    if (!jsonUtils::retrieveValue(message, ipc::AUDIO_FOCUS_MANAGER_TOKEN_TAG, &token)) {
        ACSDK_ERROR(LX("focusChangedReportFailed").d("reason", "tokenNotFound"));
        return;
    }

    handleOnFocusChangedReceivedConfirmation(token);
}

void GUIClient::authorizationStateRequest(const std::string& message) {
    if (!m_authState.empty()) {
        sendSetAuthorizationStateMessage(m_authState);
    }
}

void GUIClient::alexaStateRequest(const std::string& message) {
    if (!m_alexaState.empty()) {
        sendSetAlexaStateMessage(m_alexaState);
    }
}

void GUIClient::authorizationInfoRequest(const std::string& message) {
    // Do not send the authorization info if it is already done.
    if (m_authState != "REFRESHED" && !m_authUrl.empty() && !m_authCode.empty() && !m_clientId.empty()) {
        sendCompleteAuthorizationMessage(m_authUrl, m_authCode, m_clientId);
    }
}

void GUIClient::localesRequest(const std::string& message) {
    if (!m_localeStr.empty()) {
        sendSetLocalesMessage(m_localeStr);
    }
}

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
