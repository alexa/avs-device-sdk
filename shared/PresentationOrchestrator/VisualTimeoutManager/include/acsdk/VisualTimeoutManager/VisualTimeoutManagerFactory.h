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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_INCLUDE_ACSDK_VISUALTIMEOUTMANAGER_VISUALTIMEOUTMANAGERFACTORY_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_INCLUDE_ACSDK_VISUALTIMEOUTMANAGER_VISUALTIMEOUTMANAGERFACTORY_H_

#include <memory>

#include <acsdk/PresentationOrchestratorInterfaces/VisualTimeoutManagerInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace visualTimeoutManager {

/**
 * Class which creates instances of the @c VisualTimeoutManager
 */
class VisualTimeoutManagerFactory {
public:
    /**
     * Struct containing interfaces exposed by the @c VisualTimeoutManager
     */
    struct VisualTimeoutManagerExports {
        /// Instance of the @c VisualTimeoutManagerInterface exposed by the VisualTimeoutManager
        std::shared_ptr<presentationOrchestratorInterfaces::VisualTimeoutManagerInterface>
            visualTimeoutManagerInterface;

        /// Instance of @c RequiresShutdown used for cleaning up during shutdown.
        std::shared_ptr<avsCommon::utils::RequiresShutdown> requiresShutdown;
    };

    /**
     * Creates an instance of the VisualTimeoutManager
     * @return Optional containing the interfaces exported by the VisualTimeoutManager, or empty if creation
     * failed
     */
    static avsCommon::utils::Optional<VisualTimeoutManagerExports> create();
};

}  // namespace visualTimeoutManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_VISUALTIMEOUTMANAGER_INCLUDE_ACSDK_VISUALTIMEOUTMANAGER_VISUALTIMEOUTMANAGERFACTORY_H_
