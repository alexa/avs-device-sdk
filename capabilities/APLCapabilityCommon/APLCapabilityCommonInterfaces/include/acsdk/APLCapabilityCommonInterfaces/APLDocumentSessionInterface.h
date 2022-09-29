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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_

#include <chrono>
#include <string>

#include <AVSCommon/SDKInterfaces/ContextRequestToken.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>

#include "PresentationSession.h"
#include "PresentationOptions.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/**
 * A @c APLDocumentSessionInterface allows for controlling a previously rendered
 * APL document. All methods except @c getToken() result in asynchronous
 * operations so @c APLDocumentObserverInterface implementations should be used
 * for monitoring the success of method calls.
 */
class APLDocumentSessionInterface {
public:
    /**
     * Destructor.
     */
    virtual ~APLDocumentSessionInterface() = default;

    /**
     * Dismiss APL document.  No other functions can be called after this
     */
    virtual void clearDocument() = 0;

    /**
     * Execute commands referenced in APL document
     * @param commands JSON string containing APL command to execute
     */
    virtual void executeCommands(const std::string& commands) = 0;

    /**
     * Update the data source payload for a given APL document
     * @param sourceType APL spec source type for data update
     * @param payload JSON string containing update payload
     */
    virtual void dataSourceUpdate(const std::string& sourceType, const std::string& payload) = 0;

    /**
     * Interrupt any active command sequence currently executing on the document
     */
    virtual void interruptCommandSequence() = 0;

    /**
     * Provide visual context to onVisualContextAvailable observer callback
     * @param stateRequestToken provided by StateProviderInterface call
     */
    virtual void provideDocumentContext(const avsCommon::sdkInterfaces::ContextRequestToken stateRequestToken) = 0;

    /**
     * Request active document session to move to the foreground if not already
     * there.
     */
    virtual void requestForeground() = 0;

    /**
     * Update the timeout policy for document session.  Will reset any active
     * timeout timer.
     * @param timeout updated timeout duration, use a value of -1 to disable the timeout
     */
    virtual void updateTimeout(std::chrono::milliseconds timeout) = 0;

    /**
     * Stop any active timeout timer.
     */
    virtual void stopTimeout() = 0;

    /**
     * Reset a timeout timer based on configured timeout policy.
     */
    virtual void resetTimeout() = 0;

    /**
     * Update the @c PresentationLifespan for document session.
     * @param lifespan @c PresentationLifespan
     */
    virtual void updateLifespan(presentationOrchestratorInterfaces::PresentationLifespan lifespan) = 0;

    /**
     * Get presentation token of this document session.  Can be used for sharing
     * @c APLDocumentObserverInterface instances across multiple documents.
     */
    virtual PresentationToken getToken() const = 0;

    /**
     * Check if document session is foreground focused
     * @return true if session is foreground focused, false otherwise
     */
    virtual bool isForegroundFocused() = 0;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_
