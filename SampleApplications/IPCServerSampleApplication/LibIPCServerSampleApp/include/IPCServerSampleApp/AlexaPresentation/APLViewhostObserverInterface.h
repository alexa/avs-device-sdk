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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTOBSERVERINTERFACE_H_

#include <memory>
#include <string>

#include <APLClient/AplCommandExecutionEvent.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
/**
 * Observer interface used for callbacks from the APL Viewhost implementation.
 */
class APLViewhostObserverInterface {
public:
    /**
     * Destructor
     */
    virtual ~APLViewhostObserverInterface() = default;

    /**
     * Callback when command execution is complete.
     * @param token the presentation token corresponding to the document being executed
     * @param event the command execution event.
     * @param message the execute command completion message.
     */
    virtual void onCommandExecutionComplete(
        const std::string& token,
        APLClient::AplCommandExecutionEvent event,
        const std::string& message) = 0;

    /**
     * Callback when render document is complete.
     * @param token the presentation token corresponding to the document being rendered
     * @param result true if successful, false otherwise
     * @param timestamp The timestamp at which the document was rendered
     */
    virtual void onRenderDocumentComplete(
        const std::string& token,
        bool result,
        const std::string& error,
        const std::chrono::steady_clock::time_point& timestamp) = 0;

    /**
     * Callback for send event request
     * @param token the presentation token corresponding to the document sending the event
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
     * callback when a new visual context is available
     * @param requestToken token which was provided with the original visual context request
     * @param token the presentation token corresponding to the document providing the context.
     * @param version the version of the UI component on the device.
     * @param visualContext serialized visual context
     * @param datasourceContext serialized datasource context
     */
    virtual void onVisualContextAvailable(
        const unsigned int requestToken,
        const std::string& token,
        const std::string& version,
        const std::string& visualContext,
        const std::string& datasourceContext) {
        // no-op by default for backwards compatibility
    }

    /**
     * Callback when data source update is complete.
     * @param token the presentation token corresponding to the document being updated
     * @param result true if successful, false otherwise
     * @param error error string if available
     */
    virtual void onDataSourceUpdateComplete(const std::string& token, bool result, const std::string& error) {
        // no-op by default for backwards compatibility
    }

    /**
     * Callback when document requires a data source update
     * @param token the presentation token corresponding to the document requiring the update
     * @param dataSourceType the data source type requiring update
     * @param payload The payload of the fetch request
     */
    virtual void onDataSourceFetch(
        const std::string& token,
        const std::string& dataSourceType,
        const std::string& payload) {
        // no-op by default for backwards compatibility
    }

    /**
     * Callback when document receives a runtime error
     * @param token the presentation token corresponding to the document encountering the error
     * @param payload The payload of the error
     */
    virtual void onRuntimeError(const std::string& token, const std::string& payload) {
        // no-op by default for backwards compatibility
    }

    /**
     * Callback when document is no longer displayed
     * @param token the presentation token corresponding to the document encountering the error
     */
    virtual void onDocumentFinished(const std::string& token) {
        // no-op by default for backwards compatibility
    }

    /**
     * Callback when an open URL command occurs
     * @param token the presentation token
     * @param url the url to open
     */
    virtual void onOpenURL(const std::string& token, const std::string& url) {
        // no-op by default for compatibility
    }

    /**
     * Callback when a document has been cleared
     * @param token the presentation of the cleared document
     */
    virtual void onDocumentCleared(const std::string& token) {
        // no-op by default for compatibility
    }

    /**
     * Callback when a document timeout needs to be updated
     * @param token the presentation token
     * @param timeout updated timeout value
     */
    virtual void onSetDocumentIdleTimeout(const std::string& token, const std::chrono::milliseconds& timeout) = 0;
};

using APLViewhostObserverInterfacePtr = std::shared_ptr<APLViewhostObserverInterface>;
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_ALEXAPRESENTATION_APLVIEWHOSTOBSERVERINTERFACE_H_
