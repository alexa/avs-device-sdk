/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERERINTERFACE_H_

#include "Alerts/Renderer/RendererObserverInterface.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace alerts {
namespace renderer {

/**
 * An interface class which specifies an @c Alert renderer.
 */
class RendererInterface {
public:
    /**
     * Destructor.
     */
    virtual ~RendererInterface() = default;

    /**
     * Start rendering.  This api takes two sets of parameters - a local audio file, and a vector of urls.
     * If the urls container is empty, then the local audio file will be played for either a maximum time of one hour,
     * or until explicitly being stopped.
     *
     * If the urls are non-empty, then they will be rendered in sequence, for loopCount number of times, with a pause
     * of loopPauseInMilliseconds in between each sequence.
     *
     * If any url fails to render (for example, if the url is invalid, or the media player cannot acquire it), then
     * the renderer will default to the local audio file, with the behavior described above.
     *
     * TODO : ACSDK-389 Investigate changing explicit file paths to a std::istream-based interface.
     *
     * @param observer The observer that will receive renderer events.
     * @param audioFactory A function that produces a unique stream of audio that is used for the default if nothing
     * else is available.
     * @param urls A container of urls to be rendered per the above description.
     * @param loopCount The number of times the urls should be rendered.
     * @param loopPauseInMilliseconds The number of milliseconds to pause between rendering url sequences.
     */
    virtual void start(
        std::shared_ptr<capabilityAgents::alerts::renderer::RendererObserverInterface> observer,
        std::function<std::unique_ptr<std::istream>()> audioFactory,
        const std::vector<std::string>& urls = std::vector<std::string>(),
        int loopCount = 0,
        std::chrono::milliseconds loopPause = std::chrono::milliseconds{0}) = 0;

    /**
     * Stop rendering.
     */
    virtual void stop() = 0;
};

}  // namespace renderer
}  // namespace alerts
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_ALERTS_INCLUDE_ALERTS_RENDERER_RENDERERINTERFACE_H_
