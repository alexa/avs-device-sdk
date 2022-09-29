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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_APLRUNTIMEINTERFACE_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_APLRUNTIMEINTERFACE_H_

#include <memory>
#include <string>

#include "APLDocumentObserverInterface.h"
#include "PresentationOptions.h"
#include "PresentationSession.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

/**
 * A @c APLRuntimeInterface allows for rendering and controlling APL content.
 * A @c APLRuntimeInterface implementation may handle rendering multiple
 * documents concurrently or sequentially with each document being uniquely
 * identified by a presentation token in the @c presentationOptions struct.
 *
 * Document lifecycle events can be tracked by a @c APLDocumentObserverInterface
 * observer provided in the @c renderDocument API call.  Only events for the
 * document the caller provided will be driven to the observer.  To see
 * descriptions of these document events, please refer to the documentation of
 * each method on the observer.
 *
 * The @c onAPLDocumentSessionAvailable() method in the observer can be used to
 * capture a @c APLDocumentSessionInterface.  This session object can be used to
 * control the document (i.e. @c executeCommands(), @c clearDocument() ). Status
 * responses for session control calls are provided in @c
 * APLDocumentObserverInterface methods.
 *
 * @note An @c APLRuntimeInterface implementation must be able to support the
 * various capabilities listed at:
 * https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/presentation-apl.html.
 */
class APLRuntimeInterface {
public:
    /**
     * Destructor.
     */
    virtual ~APLRuntimeInterface() = default;

    /**
     * Render an APL document with associated metdata.  Payloads must comply with
     * the latest APL spec supported on the platform the caller is using.  See @c
     * getMaxAPLVersion() method for querying support on the platform.
     * @param document  JSON string containing APL document
     * @param data  JSON string containing data payload associated with APL
     * document
     * @param presentationSession Configuration for presentation session
     * displaying document
     * @param presentationOptions Configuration for presentation displaying
     * document
     * @param observer @c APLDocumentObserverInterface observer for monitoring
     * document lifecycle events
     */
    virtual void renderDocument(
        const std::string& document,
        const std::string& data,
        const PresentationSession& presentationSession,
        const PresentationOptions& presentationOptions,
        std::shared_ptr<APLDocumentObserverInterface> observer) = 0;

    /**
     * Get the maximum APL Version currently supported on this device. See the
     * following for descriptions of the various APL Version Specifications:
     * https://developer.amazon.com/en-US/docs/alexa/alexa-presentation-language/apl-latest-version.html
     * @return string representation of APL Specification Version
     */
    virtual std::string getMaxAPLVersion() const = 0;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_APLRUNTIMEINTERFACE_H_