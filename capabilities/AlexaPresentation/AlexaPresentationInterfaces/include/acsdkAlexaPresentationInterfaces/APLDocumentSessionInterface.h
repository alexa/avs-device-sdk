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

#ifndef ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_
#define ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_

#include <acsdkAlexaPresentationInterfaces/PresentationOptions.h>

#include <chrono>
#include <string>

namespace alexaClientSDK {
namespace acsdkAlexaPresentationInterfaces {

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
    virtual void provideDocumentContext(const unsigned int stateRequestToken) = 0;

    /**
     * Request active document session to move to the foreground if not already
     * there.
     */
    virtual void requestForeground() = 0;

    /**
     * Update the timeout policy for document session.  Will reset any active
     * timeout timer.
     * @param timeout updated timeout duration
     */
    virtual void updateTimeout(std::chrono::milliseconds timeout) = 0;

    /**
     * Get presentation token of this document session.  Can be used for sharing
     * @c APLDocumentObserverInterface instances across multiple documents.
     */
    virtual std::string getToken() const = 0;

    /**
     * Update the lifespan for document session.
     * @param lifespan updated lifespan
     */
    virtual void updateLifespan(PresentationLifespan lifespan){
        // no-op for backwards compatibility
    };
};

}  // namespace acsdkAlexaPresentationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKALEXAPRESENTATIONINTERFACES_APLDOCUMENTSESSIONINTERFACE_H_
