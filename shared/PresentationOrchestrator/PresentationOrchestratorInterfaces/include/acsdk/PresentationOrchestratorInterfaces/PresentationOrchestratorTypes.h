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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORTYPES_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORTYPES_H_

#include <string>
#include <vector>

#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
/**
 * Struct holding the metadata for a presentation
 */
struct PresentationMetadata {
    /// The endpoint holding this window, the default endpoint will be assumed if empty
    std::string endpoint;

    /// The AVS interface holding this window
    std::string interfaceName;

    /// Metadata associated with the interface
    /// For example, for the Alexa.Presentation.APL interface this should contain the presentation token
    std::string metadata;
};

/**
 * Specialization of a VC Window Instance, contains additional information for a window which is required by the
 * Presentation Orchestrator
 */
struct PresentationOrchestratorWindowInstance : public visualCharacteristicsInterfaces::WindowInstance {
    /// The set of interfaces supported by this window
    std::vector<std::string> supportedInterfaces;

    /// Where in the zOrder this window instance exists
    /// @note Windows with larger values appears above windows with smaller values
    int zOrderIndex;
};

/**
 * Contains information on the current window configuration and state
 */
struct PresentationOrchestratorWindowInfo {
    /// The window instance configuration
    PresentationOrchestratorWindowInstance configuration;

    /// The window instance state
    PresentationMetadata state;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONORCHESTRATORTYPES_H_
