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

#ifndef ACSDK_APLCAPABILITYCOMMONINTERFACES_PRESENTATIONOPTIONS_H_
#define ACSDK_APLCAPABILITYCOMMONINTERFACES_PRESENTATIONOPTIONS_H_

#include <chrono>
#include <string>
#include <vector>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>

#include "PresentationToken.h"

namespace alexaClientSDK {
namespace aplCapabilityCommonInterfaces {

struct PresentationOptions {
    /// Window ID corresponding to a window reported in Alexa.Display
    std::string windowId;

    /// The timeout for the document
    std::chrono::milliseconds timeout;

    /// The presentation token that will be reported in Alexa.Display.WindowState
    PresentationToken token;

    /// Specifies the lifespan type for this presentation
    presentationOrchestratorInterfaces::PresentationLifespan lifespan;

    /// Supported viewports
    std::string supportedViewports;

    /// Specifies the timestamp when the request to render was received
    std::chrono::steady_clock::time_point documentReceivedTimestamp;

    /// The AVS namespace associated with this presentation
    std::string interfaceName;
};

}  // namespace aplCapabilityCommonInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_APLCAPABILITYCOMMONINTERFACES_PRESENTATIONOPTIONS_H_
