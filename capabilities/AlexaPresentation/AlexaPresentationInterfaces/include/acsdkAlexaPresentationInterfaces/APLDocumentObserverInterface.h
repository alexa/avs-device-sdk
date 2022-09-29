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

#ifndef ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_
#define ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_

#include <memory>
#include <string>

#include "acsdkAlexaPresentationInterfaces/APLDocumentSessionInterface.h"

namespace alexaClientSDK {
namespace acsdkAlexaPresentationInterfaces {

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
     * @param token presentationToken of document session available
     * @param session DocumentSession object
     */
    virtual void onAPLDocumentSessionAvailable(
        const std::string& token,
        std::unique_ptr<APLDocumentSessionInterface>&& session) = 0;

    /**
     * Callback when document has finished displaying
     * @param token presentationToken of document
     */
    virtual void onDocumentFinished(const std::string& token) = 0;

    /**
     * Callback when document rendering is complete
     * @param token presentationToken of document
     * @param result true if rendering was successful, false otherwise
     * @param error error string if available
     */
    virtual void onRenderDocumentComplete(const std::string& token, bool result, const std::string& error) {
        // no-op for forward compatibility
    }

    virtual void onRenderDocumentComplete(
        const std::string& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp) {
        onRenderDocumentComplete(token, result, error);
    }
    /**
     * Callback when execute command is complete
     * @param token presentationToken of document
     * @param result true if successful, false otherwise
     * @param error error string if available
     */
    virtual void onCommandExecutionComplete(const std::string& token, bool result, const std::string& error) = 0;

    /**
     * Callback when data source update is complete
     * @param token presentationToken of document
     * @param result true if successful, false otherwise
     * @param error error string if available
     */
    virtual void onDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error) = 0;

    /**
     * Callback for send event request
     * @param token presentationToken of document
     * @param arguments Arguments array of SendEvent command
     * @param components Object containing the component of the SendEvent command
     * @param source Object detailing component which generated the command
     */
    virtual void onSendEvent(
        const std::string& token,
        const std::string& arguments,
        const std::string& components,
        const std::string& source) = 0;

    /**
     * Callback to provide information about what is currently displayed on
     * screen.  Should be provided to @c ContextManagerInterface in response to
     * provideState calls.
     * @param token presentationToken of document
     * @param requestToken token correlating context to provideState request
     * @param visualContext json payload of document visual context
     */
    virtual void onVisualContextAvailable(
        const std::string& token,
        const unsigned int requestToken,
        const std::string& visualContext) = 0;

    /**
     * Callback for data source fetch request
     * @param token presentationToken of document
     * @param dataSourceType apl defined list data source type
     * @param payload json payload of data source fetch request
     */
    virtual void onDataSourceFetch(
        const std::string& token,
        const std::string& dataSourceType,
        const std::string& payload) = 0;

    /**
     * Callback when runtime error occurs in document
     * @param token presentationToken of document
     * @param payload json payload of runtime error
     */
    virtual void onRuntimeError(const std::string& token, const std::string& payload) = 0;
};

}  // namespace acsdkAlexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTOBSERVERINTERFACE_H_