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

#ifndef ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_AUDIOACTIVITYTRACKER_H_
#define ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_AUDIOACTIVITYTRACKER_H_

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
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
 * The @c AudioActivityTracker implements the @c ActivityTrackerInterface and gets notification from the @c
 * FocusManager any activities in the audio channels.  It also implements the @c StateProviderInterface and will provide
 * to AVS the activity of the audio channels as described in Focus Management.
 */
class AudioActivityTracker
        : public avsCommon::utils::RequiresShutdown
        , public ActivityTrackerInterface
        , public avsCommon::sdkInterfaces::StateProviderInterface {
public:
    /**
     * Creates a new @c AudioActivityTracker instance.
     *
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @return A @c std::shared_ptr to the new @c AudioActivityTracker instance, or @c nullptr if the operation failed.
     */
    static std::shared_ptr<AudioActivityTracker> create(
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
    AudioActivityTracker(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * This function processes the vector of @c Channel::State and stores them inside @c m_channelsContext.
     *
     * @param channelStates The vector of @c Channel::State that has been updated as notified by the FocusManager via
     * the @c notifyOfActivityUpdates() callback.
     */
    void executeNotifyOfActivityUpdates(const std::vector<Channel::State>& channelStates);

    /**
     * This function provides updated context information for AudioActivityTracker to @c ContextManager.  This function
     * is called when @c ContextManager calls @c provideState().
     *
     * @param stateRequestToken The token @c ContextManager passed to the @c provideState() call, which will be passed
     * along to the ContextManager::setState() call.
     */
    void executeProvideState(unsigned int stateRequestToken);

    /**
     * This function returns the channelName passed in to lower case.
     *
     * @param channelName The name of the @c Channel.
     * @return The name of the @c Channel in lower case.
     */
    const std::string& executeChannelNameInLowerCase(const std::string& channelName);

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /**
     * @name Executor Thread Variables
     *
     * These member variables are only accessed by functions in the @c m_executor worker thread, and do not require any
     * synchronization.
     */
    /// @{
    /// A map to store the @c Channel::State to all the audio channels.  The key of this map is the name of the @c
    /// Channel.
    std::unordered_map<std::string, Channel::State> m_channelStates;

    /// A map to store the channel name in lower cases as required by the AudioActivity context.  The key of this map is
    /// the name of the @c Channel.
    std::unordered_map<std::string, std::string> m_channelNamesInLowerCase;
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

#endif  // ALEXA_CLIENT_SDK_AFML_INCLUDE_AFML_AUDIOACTIVITYTRACKER_H_
