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

#include <mutex>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "IPCServerSampleApp/AlexaPresentation/APLDocumentSessionManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

static const std::string TAG{"APLDocumentSessionManager"};

#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<APLDocumentSessionManager> APLDocumentSessionManager::create() {
    return std::unique_ptr<APLDocumentSessionManager>(new APLDocumentSessionManager());
}

APLDocumentSessionManager::APLDocumentSessionManager() = default;

void APLDocumentSessionManager::addDocumentSession(
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
    const std::string& token,
    std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> session) {
    ACSDK_DEBUG9(LX(__func__).d("token", token.c_str()));
    std::lock_guard<std::mutex> guard(m_mutex);
    if (findSessionByTokenLocked(token)) {
        ACSDK_WARN(LX(__func__).m("Active session already exists"));
        return;
    }
    auto presentationSessionId = generatePresentationId(presentationSession);
    m_presentationSessionIdToToken[presentationSessionId].insert(token);
    m_tokenToPresentationSessionId[token] = presentationSessionId;
    m_activeSessions.insert(std::make_pair(presentationSessionId, std::move(session)));
}

std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> APLDocumentSessionManager::
    findSessionByTokenLocked(const std::string& token) {
    auto tokenToSessionIt = m_tokenToPresentationSessionId.find(token);
    if (tokenToSessionIt == m_tokenToPresentationSessionId.end()) {
        return nullptr;
    }

    auto sessionIt = m_activeSessions.find(tokenToSessionIt->second);
    if (sessionIt == m_activeSessions.end()) {
        return nullptr;
    }

    return sessionIt->second;
}

std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> APLDocumentSessionManager::
    getDocumentSessionByToken(const std::string& token) {
    ACSDK_DEBUG9(LX(__func__).d("token", token));

    std::lock_guard<std::mutex> guard(m_mutex);
    return findSessionByTokenLocked(token);
}

void APLDocumentSessionManager::clearDocumentSession(
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) {
    std::lock_guard<std::mutex> guard(m_mutex);
    const auto presentationSessionId = generatePresentationId(presentationSession);
    ACSDK_DEBUG9(LX(__func__).d("presentationSessionId", presentationSessionId));

    for (auto& token : m_presentationSessionIdToToken[presentationSessionId]) {
        m_tokenToPresentationSessionId.erase(token);
    }
    m_presentationSessionIdToToken.erase(presentationSessionId);
    m_activeSessions.erase(presentationSessionId);
}

void APLDocumentSessionManager::invokeFunctionPerDocumentSession(
    std::function<void(const std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>&)> func) {
    std::lock_guard<std::mutex> guard(m_mutex);
    for (auto const& activeSessionEntry : m_activeSessions) {
        func(activeSessionEntry.second);
    }
}

void APLDocumentSessionManager::associateTokenWithPresentationSession(
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
    const std::string& token) {
    auto presentationSessionId = generatePresentationId(presentationSession);
    ACSDK_DEBUG9(LX(__func__).d("sessionId", presentationSessionId).d("token", token));

    if (m_activeSessions.count(presentationSessionId) == 0) {
        ACSDK_WARN(LX("associateTokenWithSessionFailed")
                       .d("reason", "session does not exist")
                       .d("presentationSessionId", presentationSessionId));
        return;
    }

    std::lock_guard<std::mutex> guard(m_mutex);
    auto tokenSessionIt = m_tokenToPresentationSessionId.find(token);
    if (tokenSessionIt != m_tokenToPresentationSessionId.end()) {
        auto existingSessionId = tokenSessionIt->second;
        if (existingSessionId == presentationSessionId) {
            // Mapping already exists and is unchanged
            return;
        }

        ACSDK_DEBUG5(LX(__func__)
                         .d("presentationSessionId", presentationSessionId)
                         .d("token", token)
                         .m("Token is already associated with session, updating mapping"));
        m_presentationSessionIdToToken[existingSessionId].erase(token);
    }

    m_tokenToPresentationSessionId[token] = presentationSessionId;
    m_presentationSessionIdToToken[presentationSessionId].insert(token);
}

std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> APLDocumentSessionManager::
    getDocumentSessionByPresentationSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) {
    auto presentationSessionId = generatePresentationId(presentationSession);
    ACSDK_DEBUG9(LX(__func__).d("sessionId", presentationSessionId));
    auto it = m_activeSessions.find(presentationSessionId);
    if (it == m_activeSessions.end()) {
        return nullptr;
    }

    return it->second;
}

std::string APLDocumentSessionManager::generatePresentationId(
    const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) const {
    // Only use the part of the id before the ':' separator
    return presentationSession.id.substr(0, presentationSession.id.find(":")) + "/" + presentationSession.skillId;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
