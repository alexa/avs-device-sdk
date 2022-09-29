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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGER_H_

#include <unordered_map>

#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSessionManagerInterface.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

class APLDocumentSessionManager : public APLDocumentSessionManagerInterface {
public:
    /**
     * Create an instance of @c APLDocumentSessionManager
     * @return pointer to @c APLDocumentSessionManager
     */
    static std::unique_ptr<APLDocumentSessionManager> create();

    /// @c APLDocumentSessionManagerInterface Functions
    /// {
    void addDocumentSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::string& token,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> session) override;
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> getDocumentSessionByPresentationSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSessionId) override;
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> getDocumentSessionByToken(
        const std::string& token) override;
    void clearDocumentSession(const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) override;
    void invokeFunctionPerDocumentSession(
        std::function<void(const std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>&)> func)
        override;
    void associateTokenWithPresentationSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::string& token) override;
    /// }

private:
    /**
     * Constructor
     */
    APLDocumentSessionManager();

    /**
     * Check if session exists for a token or not
     *
     * @param token token of document
     * @return shared_ptr to the session
     */
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> findSessionByTokenLocked(
        const std::string& token);

    /**
     * Generates an ID from the presentation session
     * @param presentationSession The presentation session
     * @return A string identifying this presentationSession
     */
    std::string generatePresentationId(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) const;

    /// mutex for session map
    std::mutex m_mutex;

    /// map from token to session id
    std::unordered_map<std::string, std::string> m_tokenToPresentationSessionId;

    /// map from session id to tokens
    std::unordered_map<std::string, std::set<std::string>> m_presentationSessionIdToToken;

    /// map of presentation session id to active sessions
    std::unordered_map<std::string, std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>>
        m_activeSessions;
};
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGER_H_
