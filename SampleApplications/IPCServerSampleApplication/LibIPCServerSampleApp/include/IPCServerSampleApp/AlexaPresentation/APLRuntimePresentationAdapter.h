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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEPRESENTATIONADAPTER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEPRESENTATIONADAPTER_H_

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <acsdk/AlexaPresentationInterfaces/AlexaPresentationCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/VisualStateProviderInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLCapabilityAgentObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLDocumentObserverInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationOptions.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationToken.h>
#include <AVSCommon/AVS/PlayerActivity.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEventObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include "IPCServerSampleApp/GUI/GUIActivityEventNotifierInterface.h"

#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSessionManager.h"
#include "IPCServerSampleApp/AlexaPresentation/APLRuntimeInterfaceImpl.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class APLRuntimePresentationAdapter
        : public aplCapabilityCommonInterfaces::APLCapabilityAgentObserverInterface
        , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface
        , public avsCommon::sdkInterfaces::GUIActivityEventObserverInterface
        , public aplCapabilityCommonInterfaces::VisualStateProviderInterface
        , public aplCapabilityCommonInterfaces::APLDocumentObserverInterface
        , public std::enable_shared_from_this<APLRuntimePresentationAdapter> {
public:
    /**
     * Create an instance of APL runtime presentation adaptor.
     * @param runtimeInterface An instance of a APL runtime implementation.
     * @param activityEventNotifier Notifier to GUI activity events.
     * @return Instance of APL runtime presentation adaptor if successful, else nullptr.
     */
    static std::shared_ptr<APLRuntimePresentationAdapter> create(
        const std::shared_ptr<APLRuntimeInterfaceImpl>& runtimeInterface,
        std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier);

    /**
     * Sets the Alexa Presentation Capability Agent to the @c APLRuntimePresentationAdapter.
     * @param alexaPresentationCA Instance of the Alexa Presentation Capability Agent.
     */
    void setAlexaPresentationCA(
        std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> alexaPresentationCA);

    /**
     * Sets the default window id to use for rendering documents that do not provide one.
     * @param windowId The default window id.
     */
    void setDefaultWindowId(const std::string& windowId);

    /**
     * Returns the APL version implemented by the runtime.
     * https://developer.amazon.com/en-US/docs/alexa/alexa-presentation-language/apl-latest-version.html
     * @return apl version.
     */
    std::string getAPLRuntimeVersion();

    /// @name AlexaPresentationObserverInterface functions
    /// @{
    void onRenderDocument(
        const std::string& document,
        const std::string& datasource,
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const std::string& windowId,
        const aplCapabilityCommonInterfaces::APLTimeoutType timeoutType,
        const std::string& interfaceName,
        const std::string& supportedViewports,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::chrono::steady_clock::time_point& receiveTime,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> agent) override;

    void onExecuteCommands(const std::string& sourceType, const aplCapabilityCommonInterfaces::PresentationToken& token)
        override;

    void onDataSourceUpdate(
        const std::string& sourceType,
        const std::string& jsonPayload,
        const aplCapabilityCommonInterfaces::PresentationToken& token) override;

    void onShowDocument(const aplCapabilityCommonInterfaces::PresentationToken& token) override;
    /// @}

    /// @name APLDocumentObserverInterface functions
    /// @{
    void onAPLDocumentSessionAvailable(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        std::unique_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>&& session) override;

    void onCommandExecutionComplete(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        alexaClientSDK::aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
        const std::string& error) override;

    void onRenderDocumentComplete(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp) override;

    void onSendEvent(const aplCapabilityCommonInterfaces::aplEventPayload::UserEvent& payload) override;

    void onVisualContextAvailable(
        const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context) override;

    void onDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error) override;

    void onDataSourceFetch(const aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch& payload) override;

    void onRuntimeError(const aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& payload) override;

    void onDocumentFinished(const std::string& token) override;

    void onActiveDocumentChanged(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const aplCapabilityCommonInterfaces::PresentationSession& session) override;

    void onSessionEnded(const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) override;

    void onActivityEvent(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const avsCommon::sdkInterfaces::GUIActivityEvent& event) override;
    /// @}

    /// @name VisualStateProviderInterface functions
    /// @{
    void provideState(
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) override;
    /// @}

    /// @name ActivityEventObserverInterface functions
    /// @{
    void onGUIActivityEventReceived(
        const std::string& source,
        const avsCommon::sdkInterfaces::GUIActivityEvent& activityEvent) override;
    /// @}

    /// @name DialogUXStateObserverInterface Functions
    /// @{
    void onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) override;
    /// @}

    void onPlayerActivityChanged(avsCommon::avs::PlayerActivity state);

    /**
     * TEST ONLY
     */
    void setExecutor(std::shared_ptr<avsCommon::utils::threading::Executor>);

    /**
     * Used to request the rendering af an APL document that has no presentation lifecycle association.
     * Once called, the client should render the document based on the
     * APL specification in the payload in structured JSON format.
     *
     * @note The payload may contain customer sensitive information and should be used with utmost care.
     * Failure to do so may result in exposing or mishandling of customer data.
     *
     * @param jsonPayload A JSON payload that maps to the schema of the Alexa.Presentation.APL.RenderDocument directive
     * and follows the APL specification.
     * @param token The APL presentation token associated with this payload.
     * @param windowId The target windowId.
     * @param receiveTime The time at which the document request was initiated, for more accurate telemetry.
     * @param interface The interface (namespace) which is associated with this document.
     * @param agent Pointer to the @c AlexaPresentationCAInterface requesting the render.
     */
    void renderDocumentWithoutPresentation(
        const std::string& jsonPayload,
        const std::string& token,
        const std::string& windowId,
        const std::chrono::steady_clock::time_point& receiveTime,
        const std::string& interface,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> agent);

    /**
     * Clears the associated document from the tracked sessions.
     * @param token The toke for the APL document.
     */
    void clearDocument(const std::string& token);

private:
    APLRuntimePresentationAdapter(
        const std::shared_ptr<APLRuntimeInterfaceImpl>& runtimeInterface,
        std::unique_ptr<APLDocumentSessionManager> sessionManager,
        std::shared_ptr<gui::GUIActivityEventNotifierInterface> activityEventNotifier);

    void executeRenderDocument(
        const std::string& document,
        const std::string& datasource,
        const aplCapabilityCommonInterfaces::PresentationToken& token,
        const std::string& windowId,
        const aplCapabilityCommonInterfaces::APLTimeoutType timeoutType,
        const std::string& interfaceName,
        const std::string& supportedViewports,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::chrono::steady_clock::time_point& receiveTime,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> agent,
        bool trackAsPresentation);
    void executeClearDocument(const std::string& token);
    void executeExecuteCommands(
        const std::string& jsonPayload,
        const aplCapabilityCommonInterfaces::PresentationToken& token);
    void executeDataSourceUpdate(
        const std::string& sourceType,
        const std::string& jsonPayload,
        const std::string& token);
    void executeRequestForeground(const std::string& token);
    void executeOnCommandExecutionComplete(
        const std::string& token,
        aplCapabilityCommonInterfaces::APLCommandExecutionEvent event,
        const std::string& message);
    void executeOnRenderDocumentComplete(
        const std::string& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp);
    void executeOnSendEvent(const aplCapabilityCommonInterfaces::aplEventPayload::UserEvent& payload);
    void executeOnVisualContextAvailable(
        const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplCapabilityCommonInterfaces::aplEventPayload::VisualContext& context);
    void executeOnDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error);
    void executeOnDataSourceFetch(
        const aplCapabilityCommonInterfaces::aplEventPayload::DataSourceFetch& dataSourceFetch);
    void executeOnRuntimeError(const aplCapabilityCommonInterfaces::aplEventPayload::RuntimeError& runtimeError);
    void executeOnDocumentFinished(const std::string& token);
    void executeProvideState(const std::string& aplToken, const unsigned int stateRequestToken);
    void executeOnPlayerActivityChanged(avsCommon::avs::PlayerActivity state);

    /**
     * Tracker for PresentationAgents to Document associations.
     *
     * Because tokens are not guaranteed to be unique for documents, this tracker helps count the number of
     * documents with the given token that are associated with the agent.
     */
    struct PresentationAgentDocumentTracker {
        /**
         * Constructor
         *
         * @param token - The token for the document being mapped to the agent.
         * @param presentationAgent - The @c AlexaPresentationCAInterface presentation agent being tracked.
         */
        explicit PresentationAgentDocumentTracker(
            std::string token,
            std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> presentationAgent,
            bool trackAsPresentation) :
                token{std::move(token)},
                presentationAgent{std::move(presentationAgent)} {
            handleOnPresentationDismissed = trackAsPresentation;
            docCount = 1;
        };

        /// The token for the agent mapping.
        std::string token;

        /// The presentation agent being tracked.
        std::weak_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> presentationAgent;

        /// The number of documents with the same token associated with the agent.
        int docCount;

        /// True if the tracker manages presentations that should be reported to AVS when dismissed.
        bool handleOnPresentationDismissed;
    };

    /**
     * An internal function to retrieve an @c PresentationAgentDocumentTracker from the m_presentationAgentTrackers map
     *
     * @param token - The token identifying the agent tracker.
     */
    std::shared_ptr<PresentationAgentDocumentTracker> executeGetPresentationAgentTrackerFromToken(
        const std::string& token);

    /**
     * An internal function to retrieve an @c AlexaPresentationCAInterface from the m_presentationAgentTrackers map
     *
     * @param token - The token identifying the agent.
     */
    std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface> executeGetPresentationAgentFromToken(
        const std::string& token);

    /**
     * An internal function to remove the token association from the m_presentationAgentTrackers map.
     * Will clean up the agent tracker if it's the last token<>document instance for the agent mapping.
     *
     * @param token - The token identifying the agent.
     */
    void executeRemoveTokenFromPresentationAgentTracker(const std::string& token);

    /**
     * Update TimeoutType for an APL Document session
     * @param aplToken Token for APL Document
     * @param timeoutType new @c TimeoutType
     */
    void executeUpdateTimeoutType(
        const std::string& aplToken,
        const aplCapabilityCommonInterfaces::APLTimeoutType& timeoutType);

    const std::shared_ptr<APLRuntimeInterfaceImpl> m_runtimeInterface;

    std::unique_ptr<APLDocumentSessionManager> m_sessionManager;

    std::shared_ptr<gui::GUIActivityEventNotifierInterface> m_activityEventNotifier;

    /// Whether Alexa is speaking or listening.
    bool m_isSpeakingOrListening;

    /// Reference to the Alexa Presentation Capability Agent.
    std::shared_ptr<alexaPresentationInterfaces::AlexaPresentationCapabilityAgentInterface> m_alexaPresentationCA;

    /// The map of active tokens to @c PresentationAgentDocumentTrackers.
    std::unordered_map<std::string, std::shared_ptr<PresentationAgentDocumentTracker>> m_presentationAgentTrackers;

    /// A count of the number of presentation trackers per APL Capability Agent
    std::unordered_map<std::shared_ptr<aplCapabilityCommonInterfaces::APLCapabilityAgentInterface>, unsigned int>
        m_presentationTrackersPerCA;

    /// Executor to perform tasks asynchronously.
    std::shared_ptr<avsCommon::utils::threading::Executor> m_executor;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLRUNTIMEPRESENTATIONADAPTER_H_
