/*
 * StateSynchronizer.h
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

#ifndef ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_STATE_SYNCHRONIZER_H_
#define ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_STATE_SYNCHRONIZER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/StateSynchronizerObserverInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

class StateSynchronizer :
    public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface,
    public avsCommon::sdkInterfaces::ContextRequesterInterface,
    public std::enable_shared_from_this<StateSynchronizer> {
public:
    /// Alias for @c StateSynchronizerObserverInterface for brevity.
    using ObserverInterface = avsCommon::sdkInterfaces::StateSynchronizerObserverInterface;

    /**
     * Create an instance of StateSynchronizer.
     *
     * @param contextManager The @c ContextManagerInterface used to generate system context for events.
     * @param messageSender The @c MessageSenderInterface that sends events to AVS.
     * @return @c nullptr if the inputs are not defined, else a new instance of @c StateSynchronizer.
     */
    static std::shared_ptr<StateSynchronizer> create(
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(
            const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
            const avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;
    /// @}
    
    /// @name ContextRequesterInterface Functions
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /**
     * Add a @c StateSynchronizerObserverInterface to be notified.
     *
     * @param observer The @c StateSynchronizerObserverInterface to be added
     * @note The added observer (if it's not added before) will immediately get @c onStateChanged callback with the
     * current state of @c StateSynchronizer.
     */
    void addObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * Remove a @c StateSynchronizerObserverInterface from the list of notifiers.
     *
     * @param observer The @c StateSynchronizerObserverInterface to be removed.
     */
    void removeObserver(std::shared_ptr<ObserverInterface> observer);

    /**
     * Manage completion of event being sent.
     *
     * @param messageStatus The status of submitted @c MessageRequest.
     */
    void messageSent(avsCommon::avs::MessageRequest::Status messageStatus);

    /**
     * Shutdown sequence for the instance.
     *
     * Performing all cleanup operations to prepare the object for destruction.  This function must be called prior to
     * destruction to properly clean up the instance.
     */
    void shutdown();
private:
    /**
     * Constructor.
     *
     * @param contextManager The AVS Context manager used to generate system context for events.
     * @param messageSender The message sender interface that sends events to AVS.
     */
    StateSynchronizer(
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
            std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * Notify the observers. This function locks both @c m_stateMutex and @ m_observerMutex so these locks should not be
     * held when calling this function.
     */
    void notifyObservers();

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// The @c ContextManager used to generate system context for events.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The set of @c StateSynchronizerObserverInterface objects that will be notified upon synchronization.
    std::unordered_set<std::shared_ptr<ObserverInterface>> m_observers;

    /// The mutex to synchronize access to @c m_observers.
    std::mutex m_observerMutex;

    /// The flag that describes if the connection is alive.
    std::atomic_bool m_isConnected;

    /// The current state of @c StateSynchronizer.
    ObserverInterface::State m_state;

    /// The mutex to synchronize access to @c m_state.
    std::mutex m_stateMutex;
};


} // namespace system
} // namespace capabilityAgents
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_CAPABILITY_AGENTS_SYSTEM_INCLUDE_SYSTEM_STATE_SYNCHRONIZER_H_
