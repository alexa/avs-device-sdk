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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONINTERFACE_H_

#include <chrono>
#include <string>

#include "PresentationTypes.h"

namespace alexaClientSDK {
namespace presentationOrchestratorInterfaces {
/**
 * Interface which represents a Presentation object, provides methods to manage the lifecycle of a presentation
 */
class PresentationInterface {
public:
    /**
     * Destructor
     */
    virtual ~PresentationInterface() = default;

    /**
     * Gets a value which can be used in @c setTimeout that indicates the timeout should be disabled.
     *
     * @return Value indicating timeout is disabled
     */
    static const std::chrono::milliseconds getTimeoutDisabled() {
        static const std::chrono::milliseconds TIMEOUT_DISABLED = std::chrono::milliseconds::max();
        return TIMEOUT_DISABLED;
    }

    /**
     * Gets a value which can be used in @c setTimeout that indicates the timeout should be set to the default value
     * associated with the @c PresentationLifespan for the presentation.
     *
     * @return Value indicating default timeout should be used
     */
    static const std::chrono::milliseconds getTimeoutDefault() {
        static const std::chrono::milliseconds TIMEOUT_DEFAULT{-1};
        return TIMEOUT_DEFAULT;
    }

    /**
     * Dismiss the current presentation
     */
    virtual void dismiss() = 0;

    /**
     * Request foregrounding of the current presentation
     */
    virtual void foreground() = 0;

    /**
     * Sets the metadata associated with this presentation
     * @param token The new metadata
     */
    virtual void setMetadata(const std::string& metadata) = 0;

    /**
     * Sets the lifespan of the presentation
     * @param lifespan new lifespan value
     */
    virtual void setLifespan(const PresentationLifespan& lifespan) = 0;

    /**
     * Starts or Restarts the timeout for this presentation.
     * @note the timer will only be started if the presentation currently is focused, i.e. is in the FOREGROUND state
     */
    virtual void startTimeout() = 0;

    /**
     * Stops the timeout for this presentation
     * @note the timer may be resumed as a result of other user actions, or presentation state change events.
     */
    virtual void stopTimeout() = 0;

    /**
     * Sets the timeout for this presentation - the new value will take effect the next time the timeout is restarted
     * @param timeout The new timeout value, use @c PresentationInterface::getTimeoutDisabled() to disable the timeout
     * use @c PresentationInterface::getTimeoutDefault() to default timeout based on the presentation lifespan
     */
    virtual void setTimeout(const std::chrono::milliseconds& timeout) = 0;

    /**
     * Gets the current state for this presentation
     * @return The current presentation state
     */
    virtual PresentationState getState() = 0;
};
}  // namespace presentationOrchestratorInterfaces
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORINTERFACES_INCLUDE_ACSDK_PRESENTATIONORCHESTRATORINTERFACES_PRESENTATIONINTERFACE_H_
