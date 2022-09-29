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
#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTINTERFACE_H_

#include <chrono>
#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/ContextRequestToken.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>

#include "PresentationSession.h"
#include "APLEventPayload.h"
#include "PresentationToken.h"
#include "APLCommandExecutionEvent.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {
/**
 * Defines a Contract for clients to communicate with the APL Capability agent.
 */
class APLCapabilityAgentInterface {
public:
    /**
     * Destructor.
     */
    virtual ~APLCapabilityAgentInterface() = default;

    /**
     * This function notifies the APL CA that the active document has been replaced.
     * @note A change to the active document does not indicate that the previous document was dismissed.
     * @param token The new active document token
     * @param session The new presentation session
     */
    virtual void onActiveDocumentChanged(const PresentationToken& token, const PresentationSession& session) = 0;

    /**
     * This function clears last received @c ExecuteCommands directive (if it's still active) and mark them as failed.
     * @param token The token. This should be passed in if we are clearing execute commands due to APL-specific trigger
     * (eg. Finish command). This should be left empty if we are clearing due to global triggers (eg. back navigation)
     * @param markAsFailed Whether to mark the cleared commands as failed.
     */
    virtual void clearExecuteCommands(
        const PresentationToken& token = std::string(),
        const bool markAsFailed = true) = 0;

    /**
     * Send @c UserEvent to AVS
     *
     * @param payload The @c UserEvent event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::UserEvent.
     */
    virtual void sendUserEvent(const aplEventPayload::UserEvent& payload) = 0;

    /**
     * Send @c DataSourceFetchRequest to AVS
     *
     * @param payload The @c DataSourceFetchRequest event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::DataSourceFetch.
     */
    virtual void sendDataSourceFetchRequestEvent(const aplEventPayload::DataSourceFetch& payload) = 0;

    /**
     * Send @c RuntimeError to AVS
     *
     * @param payload The @c RuntimeError event payload. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::RuntimeError.
     */
    virtual void sendRuntimeErrorEvent(const aplEventPayload::RuntimeError& payload) = 0;

    /**
     * This function is called by the clients to provide the visual context to be passed to AVS.
     * @param requestToken The token of the request for which this function is called. This should match the request
     * token provided in @c VisualStateProviderInterface
     * @param context The visual state to be passed to AVS. The caller of this
     * function is responsible to pass the payload as it defined in @c aplEventPayload::VisualContext
     */
    virtual void onVisualContextAvailable(
        avsCommon::sdkInterfaces::ContextRequestToken requestToken,
        const aplEventPayload::VisualContext& visualContext) = 0;

    /**
     * Process result of RenderDocument directive.
     *
     * @param token document presentationToken.
     * @param result rendering result (true on rendered, false on exception).
     * @param error error message provided in case if result is false
     */
    virtual void processRenderDocumentResult(
        const PresentationToken& token,
        const bool result,
        const std::string& error) = 0;

    /**
     * Process result of ExecuteCommands directive.
     *
     * @param token request token
     * @param event the command result as defined in @c APLCommandExecutionEvent
     * @param error error message provided in case if result is false
     */
    virtual void processExecuteCommandsResult(
        const PresentationToken& token,
        APLCommandExecutionEvent event,
        const std::string& error) = 0;

    /**
     * Record the finish event for currently rendering document
     * @param timestamp The timestamp at which the document was rendered
     */
    virtual void recordRenderComplete(const std::chrono::steady_clock::time_point& timestamp) = 0;

    /**
     * The function notifies the CA to initiate a proactive state report
     */
    virtual void proactiveStateReport() = 0;
};
}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK
#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLCAPABILITYAGENTINTERFACE_H_