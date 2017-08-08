/*
 * RendererObserverInterface.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_OBSERVER_INTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_OBSERVER_INTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

/**
 * An interface class which specifies an observer to an @c Alert renderer.
 */
class RendererObserverInterface {
public:
    /**
     * The states which a Renderer may be in.
     */
    enum class State {
        /// An uninitialized value.
        UNSET,
        /// The renderer has started rendering.
        STARTED,
        /// The renderer has stopped rendering due to being stopped via a direct api call.
        STOPPED,
        /// The renderer has encountered an error.
        ERROR
    };

    /**
     * Destructor.
     */
    virtual ~RendererObserverInterface() = default;

    /**
     * A callback function to communicate a change in render state.
     *
     * @param state The current state of the renderer.
     * @param reason The reason for the change of state, if required.  This is typically set on an error.
     */
    virtual void onRendererStateChange(State state, const std::string & reason = "") = 0;
};

} // namespace renderer
} // namespace alerts
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERER_OBSERVER_INTERFACE_H_