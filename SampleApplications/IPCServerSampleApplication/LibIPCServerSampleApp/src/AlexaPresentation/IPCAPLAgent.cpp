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

#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>

#include "IPCServerSampleApp/AlexaPresentation/IPCAPLAgent.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace aplCapabilityCommonInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG("IPCAPLAgent");

/**
 * Create a LogEntry using this file's TAG and the specified event string
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<IPCAPLAgent> IPCAPLAgent::create(std::shared_ptr<gui::GUIClientInterface> guiClient) {
    if (!guiClient) {
        ACSDK_CRITICAL(LX(__func__).d("reason", "null guiClient"));
        return nullptr;
    }

    return std::shared_ptr<IPCAPLAgent>(new IPCAPLAgent(guiClient));
}

IPCAPLAgent::IPCAPLAgent(std::shared_ptr<gui::GUIClientInterface> guiClient) {
    m_guiClient = std::move(guiClient);
}
void IPCAPLAgent::setAPLMaxVersion(const std::string& APLMaxVersion) {
    ACSDK_DEBUG5(LX(__func__).d("APLMaxVersion", APLMaxVersion));
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::onActiveDocumentChanged(const std::string& token, const PresentationSession& session) {
    ACSDK_DEBUG5(LX(__func__).d("token", token));
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::onDocumentDismissed(const std::string& token) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::addObserver(std::shared_ptr<APLCapabilityAgentObserverInterface> observer) {
    // No-op
}
void IPCAPLAgent::removeObserver(std::shared_ptr<APLCapabilityAgentObserverInterface> observer) {
    // No-op
}
void IPCAPLAgent::clearExecuteCommands(const std::string& token, const bool markAsFailed) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::sendUserEvent(const aplEventPayload::UserEvent& payload) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::sendDataSourceFetchRequestEvent(const aplEventPayload::DataSourceFetch& fetchPayload) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::sendRuntimeErrorEvent(const aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& payload) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::onVisualContextAvailable(
    avsCommon::sdkInterfaces::ContextRequestToken requestToken,
    const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& visualContext) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::processRenderDocumentResult(const std::string& token, const bool result, const std::string& error) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::processExecuteCommandsResult(
    const std::string& token,
    APLCommandExecutionEvent event,
    const std::string& error) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::recordRenderComplete(const std::chrono::steady_clock::time_point& timestamp) {
    // TODO : Implement IPC Message to GUIClient
}
void IPCAPLAgent::proactiveStateReport() {
    // TODO : Implement IPC Message to GUIClient
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
