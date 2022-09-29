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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGERINTERFACE_H_

#include <functional>
#include <memory>
#include <string>

#include <acsdk/APLCapabilityCommonInterfaces/APLDocumentSessionInterface.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

/**
 * A @c APLDocumentSessionManagerInterface allows for tracking the active APL Document Sessions
 */
class APLDocumentSessionManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~APLDocumentSessionManagerInterface() = default;

    /**
     * Add an active session
     *
     * @param presentationSession presentation session for this document session
     * @param token token of document session
     * @param session the document session object
     */
    virtual void addDocumentSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSessionId,
        const std::string& token,
        std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> session) = 0;

    /**
     * Get the Session using the Presentation Session
     *
     * @param presentationSession The presentation session
     * @return shared_ptr to the @c APLDocumentSessionInterface associated with this presentationSession, or nullptr if
     * none exists
     */
    virtual std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>
    getDocumentSessionByPresentationSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) = 0;

    /**
     * Get the Session using the APL token
     *
     * @param token token of document session
     * @return shared_ptr to the @c APLDocumentSessionInterface associated with this token, or nullptr if none exists
     */
    virtual std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface> getDocumentSessionByToken(
        const std::string& token) = 0;

    /**
     * Clear an active document session
     *
     * @param presentationSession presentation session of document session to clear
     */
    virtual void clearDocumentSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession) = 0;

    /**
     * Invoke a function on each document session tracked by the session manager
     *
     * @param func function to be executed with the document session
     */
    virtual void invokeFunctionPerDocumentSession(
        std::function<void(const std::shared_ptr<aplCapabilityCommonInterfaces::APLDocumentSessionInterface>&)>
            func) = 0;

    /**
     * Associates a token with an existing presentation session
     * @param presentationSession The presentation session, must have already been added with @c addDocumentSession
     * @param token The document token
     */
    virtual void associateTokenWithPresentationSession(
        const aplCapabilityCommonInterfaces::PresentationSession& presentationSession,
        const std::string& token) = 0;
};

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLDOCUMENTSESSIONMANAGERINTERFACE_H_
