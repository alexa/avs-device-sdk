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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/ContextRequestToken.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>

#include "APLDocumentSessionInterface.h"
#include "APLCommandExecutionEvent.h"
#include "APLEventPayload.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/**
 * A @c APLDocumentObserverInterface allows for observing the lifecycle of
 * rendered APL documents.
 */
class APLDocumentObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~APLDocumentObserverInterface() = default;

    /**
     * Callback when document session is available.  Session may be stored to
     * further influence the document being rendered.
     * @param presentationSession The presentation session associated with this document session
     * @param token presentationToken of document session available
     * @param session DocumentSession object
     */
    virtual void onAPLDocumentSessionAvailable(
        const PresentationSession& presentationSession,
        const PresentationToken& token,
        std::unique_ptr<APLDocumentSessionInterface>&& session) = 0;

    /**
     * Callback when document has finished displaying
     * @param token presentationToken of document
     */
    virtual void onDocumentFinished(const PresentationToken& token) = 0;

    /**
     * Callback when the active document has changed
     * @param token APL Token for the new active document
     * @param session The @c PresentationSession for the new active document
     */
    virtual void onActiveDocumentChanged(const PresentationToken& token, const PresentationSession& session) = 0;

    /**
     * Callback when document rendering is complete
     * @param token presentationToken of document
     * @param result true if rendering was successful, false otherwise
     * @param error error string if available
     * @param timestamp The timestamp at which the document was rendered
     */
    virtual void onRenderDocumentComplete(
        const PresentationToken& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp) = 0;

    /**
     * Callback when execute command is complete
     * @param token presentationToken of document
     * @param result the command result as defined in @c APLCommandExecutionEvent
     * @param error error string if available
     */
    virtual void onCommandExecutionComplete(
        const PresentationToken& token,
        APLCommandExecutionEvent result,
        const std::string& error) = 0;

    /**
     * Callback when data source update is complete
     * @param token presentationToken of document
     * @param result true if successful, false otherwise
     * @param error error string if availabled
     */
    virtual void onDataSourceUpdateComplete(const PresentationToken& token, bool result, const std::string& error) = 0;

    /**
     * Callback for send event request
     *
     * @param payload The @c UserEvent event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::UserEvent.
     */
    virtual void onSendEvent(const aplEventPayload::UserEvent& payload) = 0;

    /**
     * Callback to provide information about what is currently displayed on
     * screen.  Should be provided to @c ContextManagerInterface in response to
     * provideState calls.
     *
     * @param requestToken token correlating context to provideState request
     * @param context The visual state to be passed to AVS. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::VisualContext
     */
    virtual void onVisualContextAvailable(
        const avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplEventPayload::VisualContext& context) = 0;

    /**
     * Callback for data source fetch request
     *
     * @param payload The @c DataSourceFetchRequest event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::DataSourceFetch.
     */
    virtual void onDataSourceFetch(const aplEventPayload::DataSourceFetch& payload) = 0;

    /**
     * Callback when runtime error occurs in document
     *
     * @param payload The @c RuntimeError event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::RuntimeError.
     */
    virtual void onRuntimeError(const aplEventPayload::RuntimeError& payload) = 0;

    /**
     * Callback when a document session has ended
     * @param presentationSession The session that was associated with this document session
     */
    virtual void onSessionEnded(const PresentationSession& presentationSession) = 0;

    /**
     * Callback when a document receives a GUI activity
     * @param token presentationToken of document
     * @param event The @c GUIActivityEvent event
     */
    virtual void onActivityEvent(
        const PresentationToken& token,
        const avsCommon::sdkInterfaces::GUIActivityEvent& event) = 0;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_
