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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSION_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSION_H_

#include <chrono>
#include <string>
#include <unordered_set>

#include <AVSCommon/Utils/Threading/Executor.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLDocumentSessionInterface.h>
#include <acsdk/APLCapabilityCommonInterfaces/APLTimeoutType.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationOptions.h>
#include <acsdk/APLCapabilityCommonInterfaces/PresentationSession.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationObserverInterface.h>
#include "IPCServerSampleApp/AlexaPresentation/APLViewhostInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
class APLDocumentSession
        : public aplCapabilityCommonInterfaces::APLDocumentSessionInterface
        , public presentationOrchestratorInterfaces::PresentationObserverInterface
        , public APLViewhostObserverInterface
        , public std::enable_shared_from_this<APLDocumentSession> {
public:
    /**
     * Constructor
     * @param document APL document for this session.
     * @param data APL data for this session.
     * @param supportedViewPorts Supported viewports for this session.
     * @param presentationSession Presentation session for this session.
     * @param presentationOptions Presentation options for this session.
     * @param observer Observer of this document session.
     * @param viewhost Instance of the viewhost on which the document session will be rendered.
     * @param hasPresentationAssociation Boolean defining whether session has associated presentation.
     */
    APLDocumentSession(
        const std::string& document,
        const std::string& data,
        const std::string& supportedViewPorts,
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions,
        std::shared_ptr<alexaClientSDK::aplCapabilityCommonInterfaces::APLDocumentObserverInterface> observer,
        APLViewhostInterfacePtr viewhost,
        bool hasPresentationAssociation);

    void renderDocument(
        const std::string& document,
        const std::string& data,
        const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const alexaClientSDK::aplCapabilityCommonInterfaces::PresentationOptions& presentationOptions);

    void updateTimeoutType(const aplCapabilityCommonInterfaces::APLTimeoutType& timeoutType);

    /// @name APLDocumentSessionInterface Functions
    /// {
    void clearDocument() override;
    void executeCommands(const std::string& commands) override;
    void dataSourceUpdate(const std::string& sourceType, const std::string& payload) override;
    void interruptCommandSequence() override;
    void provideDocumentContext(const unsigned int stateRequestToken) override;
    void requestForeground() override;
    void stopTimeout() override;
    void resetTimeout() override;
    void updateLifespan(presentationOrchestratorInterfaces::PresentationLifespan lifespan) override;
    void updateTimeout(std::chrono::milliseconds timeout) override;
    std::string getToken() const override;
    bool isForegroundFocused() override;
    /// }

    /// @name PresentationObserverInterface functions
    /// @{
    void onPresentationAvailable(
        presentationOrchestratorInterfaces::PresentationRequestToken id,
        std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> presentation) override;
    void onPresentationStateChanged(
        presentationOrchestratorInterfaces::PresentationRequestToken id,
        presentationOrchestratorInterfaces::PresentationState newState) override;
    bool onNavigateBack(presentationOrchestratorInterfaces::PresentationRequestToken id) override;
    /// @}

    /// @name APLViewhostObserverInterface functions
    /// @{
    void onCommandExecutionComplete(
        const std::string& token,
        APLClient::AplCommandExecutionEvent event,
        const std::string& message) override;
    void onRenderDocumentComplete(
        const std::string& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp) override;
    void onSendEvent(
        const std::string& token,
        const std::string& arguments,
        const std::string& components,
        const std::string& source) override;
    void onVisualContextAvailable(
        const unsigned int requestToken,
        const std::string& token,
        const std::string& version,
        const std::string& visualContext,
        const std::string& datasourceContext) override;
    void onDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error) override;
    void onDataSourceFetch(const std::string& token, const std::string& dataSourceType, const std::string& payload)
        override;
    void onRuntimeError(const std::string& token, const std::string& payload) override;
    void onDocumentFinished(const std::string& token) override;
    void onOpenURL(const std::string& token, const std::string& url) override;
    void onDocumentCleared(const std::string& token) override;
    void onSetDocumentIdleTimeout(const std::string& token, const std::chrono::milliseconds& timeout) override;
    /// @}

    /**
     * Inits the first render of the session
     */
    void firstRender();

    /**
     * Helper function which can be used to convert a @c APLDocumentSessionInterface to an @c APLDocumentSession
     * @param aplDocumentSessionInterface The interface to try to convert
     * @return A pointer to the APLDocumentSession, or nullptr if @c aplDocumentSessionInterface does not represent an
     * @c APLDocumentSession
     */
    static std::shared_ptr<APLDocumentSession> getDocumentSessionFromInterface(
        const std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>& aplDocumentSessionInterface);

private:
    /**
     * This class wraps the APLDocumentSession to allow this implementation to comply with the
     * APLDocumentObserverInterface
     */
    class APLDocumentSessionWrapper : public aplCapabilityCommonInterfaces::APLDocumentSessionInterface {
    public:
        /**
         * Constructor
         * @param documentSession Pointer to the document session
         */
        APLDocumentSessionWrapper(std::shared_ptr<APLDocumentSession> documentSession);

        /// @name APLDocumentSessionInterface Functions
        /// {
        void clearDocument() override;
        void executeCommands(const std::string& commands) override;
        void dataSourceUpdate(const std::string& sourceType, const std::string& payload) override;
        void interruptCommandSequence() override;
        void provideDocumentContext(const unsigned int stateRequestToken) override;
        void requestForeground() override;
        void stopTimeout() override;
        void resetTimeout() override;
        void updateLifespan(presentationOrchestratorInterfaces::PresentationLifespan lifespan) override;
        void updateTimeout(std::chrono::milliseconds timeout) override;
        std::string getToken() const override;
        bool isForegroundFocused() override;
        /// }

        /**
         * Conversion operator to obtain the underlying document session
         * @return The underlying document session
         */
        operator std::shared_ptr<APLDocumentSession> const();

    private:
        /// The document session
        std::shared_ptr<APLDocumentSession> m_documentSession;
    };

    /**
     * Renders a document, must only be executed from inside the executor thread
     * @param firstRender Whether this is the first time this DocumentSession is rendering
     * @return true if the render request was made to the viewhost
     */
    bool executeRenderDocument(bool firstRender = false);

    /**
     * Clears the currently rendering document, must only be executed from inside the executor thread
     * @param forceClear Whether the presentation state check should be skipped
     */
    void executeClearDocument(bool forceClear = false);

    /**
     * Checks if this document session is still associated with a valid presentation
     * @return true if document session is still active
     */
    bool executeIsPresentationActive() const;

    /**
     * Checks if this document session is still associated with a presentation that is valid and foregrounded.
     * @return true if document session is in foreground.
     */
    bool executeIsPresentationForegrounded() const;

    /**
     * Checks if this APLDocumentSession is aware of this APL token
     * @param token The APL token to check
     * @return true if the token is handled by this document session, false otherwise
     */
    bool canHandleToken(const std::string& token);

    /// A weak pointer to the APL Client/Viewhost.
    std::weak_ptr<APLViewhostInterface> m_viewhost;

    /// Indicates if this document session has an association with a presentation.
    bool m_hasPresentationAssociation;

    /// Pointer to associated presentation
    std::shared_ptr<presentationOrchestratorInterfaces::PresentationInterface> m_presentation;

    /// The current presentation state
    presentationOrchestratorInterfaces::PresentationState m_state;

    /// The last rendered APL document
    std::string m_document;

    /// The APL document data
    std::string m_data;

    /// The presentation session for this APLDocumentSession as provided during construction or renderDocument
    aplCapabilityCommonInterfaces::PresentationSession m_presentationSession;

    /// The presentation options for this APLDocumentSession as provided during construction or renderDocument
    aplCapabilityCommonInterfaces::PresentationOptions m_presentationOptions;

    /// The observer for this APLDocumentSession
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentObserverInterface> m_observer;

    /// The set of tokens associated with this APLDocumentSession
    std::unordered_set<std::string> m_tokens;

    /// A string containing the json value of supported viewports.
    std::string m_supportedViewports;

    /// Whether this document session has completed rendering
    bool m_renderComplete;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    avsCommon::utils::threading::Executor m_executor;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSION_H_
