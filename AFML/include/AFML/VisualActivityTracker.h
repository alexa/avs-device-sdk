/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_VISUALACTIVITYTRACKER_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_VISUALACTIVITYTRACKER_H_

#include <chrono>
#include <memory>
#include <vector>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/StateProviderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "AFML/Channel.h"
#include "AFML/ActivityTrackerInterface.h"

namespace alexaClientSDK {
namespace afml {

/**
 * The @c VisualActivityTracker implements the @c ActivityTrackerInterface and gets notification from the @c
 * FocusManager any activities in the visual channels.  It also implements the @c StateProviderInterface and will
 * provide to AVS the activity of the visual channels as described in Focus Management.
 */
class VisualActivityTracker
        : public avsCommon::utils::RequiresShutdown
        , public ActivityTrackerInterface
        , public avsCommon::sdkInterfaces::StateProviderInterface {
public:
    /**
     * Creates a new @c VisualActivityTracker instance.
     *
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @return A @c std::shared_ptr to the new @c VisualActivityTracker instance, or @c nullptr if the operation failed.
     */
    static std::shared_ptr<VisualActivityTracker> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(const avsCommon::avs::NamespaceAndName& stateProviderName, unsigned int stateRequestToken)
        override;
    /// @}

    /// @name ActivityTrackerInterface Functions
    /// @{
    void notifyOfActivityUpdates(const std::vector<Channel::State>& channelStates) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contextManager The AVS Context manager used to generate system context for events.
     */
    VisualActivityTracker(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * This function provides updated context information for VisualActivityTracker to @c ContextManager.  This function
     * is called when @c ContextManager calls @c provideState().
     *
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     * along to the ContextManager::setState() call.
     */
    void executeProvideState(unsigned int stateRequestToken);

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// Stores the @c Channel::State to the visual channels.
    Channel::State m_channelState;
    /// @}

    /**
     * @c Executor which queues up operations from asynchronous API calls.
     *
     * @note This declaration needs to come *after* the Executor Thread Variables above so that the thread shuts down
     *     before the Executor Thread Variables are destroyed.
     */
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_VISUALACTIVITYTRACKER_H_
