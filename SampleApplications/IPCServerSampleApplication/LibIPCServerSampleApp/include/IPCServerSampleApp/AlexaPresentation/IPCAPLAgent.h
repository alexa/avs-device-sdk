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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_IPCAPLAGENT_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_IPCAPLAGENT_H_

#include <memory>
#include <string>

#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCommandExecutionEvent.h>

#include "IPCServerSampleApp/GUI/GUIClientInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * IPC APL Agent used for routing APL runtime events from documents initiated by an IPC client.
 */
class IPCAPLAgent : public aplCapabilityCommonInterfaces::APLCapabilityAgentInterface {
public:
    /**
     * Create a IPCAPLAgent
     *
     * @param guiClient A pointer to a GUI Client.
     * @return an instance of IPCAPLAgent.
     */
    static std::shared_ptr<IPCAPLAgent> create(std::shared_ptr<gui::GUIClientInterface> guiClient);

    /**
     * Destructor.
     */
    virtual ~IPCAPLAgent() = default;

    /// @name APLCapabilityAgentInterface functions
    /// @{
    void setAPLMaxVersion(const std::string& APLMaxVersion);

    void onDocumentDismissed(const std::string& token);

    void onActiveDocumentChanged(
        const std::string& token,
        const aplCapabilityCommonInterfaces::PresentationSession& session) override;

    void addObserver(std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer);

    void removeObserver(std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface> observer);

    void clearExecuteCommands(const std::string& token = std::string(), bool markAsFailed = true) override;

    void sendUserEvent(
        const alexaClientSDK::aplCapabilityCommonInterfaces::aplEventPayload::UserEvent& payload) override;

    void sendDataSourceFetchRequestEvent(
        const alexaClientSDK::aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch& fetchPayload) override;

    void sendRuntimeErrorEvent(
        const alexaClientSDK::aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& payload) override;

    void onVisualContextAvailable(
        alexaClientSDK::avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const alexaClientSDK::aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& visualContext) override;

    void processRenderDocumentResult(const std::string& token, const bool result, const std::string& error) override;

    void processExecuteCommandsResult(
        const std::string& token,
        alexaClientSDK::aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
        const std::string& error) override;

    void recordRenderComplete(const std::chrono::steady_clock::time_point& timestamp) override;

    void proactiveStateReport() override;
    /// @}

private:
    explicit IPCAPLAgent(std::shared_ptr<gui::GUIClientInterface> guiClient);

    /// Pointer to the GUI Client, used for sending IPC messages
    std::shared_ptr<gui::GUIClientInterface> m_guiClient;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_IPCAPLAGENT_H_
