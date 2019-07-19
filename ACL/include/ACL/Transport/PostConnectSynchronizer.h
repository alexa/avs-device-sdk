/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "ACL/Transport/PostConnectInterface.h"
#include "ACL/Transport/PostConnectSendMessageInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"

namespace alexaClientSDK {
namespace acl {

class HTTP2Transport;

/**
 * Class that posts the StateSynchronizer message to the AVS.
 */
class PostConnectSynchronizer
        : public PostConnectInterface
        , public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public std::enable_shared_from_this<PostConnectSynchronizer> {
public:
    /**
     * Creates a PostConnectSynchronizer object.
     *
     * @param The @c ContextManager which will provide the context for the @c SynchronizeState event.
     * @return a shared pointer to PostConnectSynchronizer if all given parameters are valid, otherwise return nullptr.
     */
    static std::shared_ptr<PostConnectSynchronizer> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    ~PostConnectSynchronizer();

    /// @name PostConnectInterface methods
    /// @{
    bool doPostConnect(std::shared_ptr<HTTP2Transport> transport) override;
    void onDisconnect() override;
    ///@}

    /// @name ContextRequesterInterface methods
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// @name MessageRequestObserverInterface methods
    /// @{
    void onSendCompleted(MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& exceptionMessage) override;
    /// @}

private:
    /**
     * Enum specifying the various states in a @c PostConnectSychronizer's lifecycle.
     */
    enum class State {
        /// @c mainLoop() is not running.
        IDLE,

        /// @c mainLoop() is running, no @c GetContext() request or message send is in progress.
        RUNNING,

        /// @c mainLoop() is running, and a @c getContext() request is in progress.
        FETCHING,

        /// @c mainLoop() is running, and a message send is in progress.
        SENDING,

        /// @c mainLoop() is running, but has been instructed to stop.
        STOPPING,

        /// @ mainLoop has exited.
        STOPPED
    };

    /// Friend to allow access to enum class @c State.
    friend std::ostream& operator<<(std::ostream& stream, State state);

    /**
     * PostConnectSynchronizer Constructor.
     *
     * @param contextManager The @c ContextManager which will provide the context for the @c SynchronizeState event.
     */
    PostConnectSynchronizer(std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /**
     * Set the next state of this @c PostConnectSynchrnizer.
     *
     * @param state The new state to transition to.
     * @return Whether or not the transition was allowed.
     */
    bool setState(State state);

    /**
     * Set the next state of this @c PostConnectSynchrnizer.
     * @note m_mutex must be held when this method is called.
     *
     * @param state The new state to transition to.
     * @return Whether or not the transition was allowed.
     */
    bool setStateLocked(State state);

    /**
     * Loop to fetch the context and send a post-connect message to AVS - or retry if either fails.
     */
    void mainLoop();

    /**
     * Stop mainLoop().  This method blocks until mainLoop() exits or the operation fails.
     *
     * @return Whether the operation was successful.
     */
    bool stop();

    /**
     * Get @c m_transport in a thread-safe manner.
     *
     * @return The current value of m_transport.
     */
    std::shared_ptr<HTTP2Transport> getTransport();

    /**
     * Set the value of @c m_transport.
     *
     * @param transport The new value for @c m_transport.
     */
    void setTransport(std::shared_ptr<HTTP2Transport> transport);

    /// Serializes access to members.
    std::mutex m_mutex;

    /// The current state of this @c PostConnectSynchronizer.
    State m_state;

    /// Condition variable on which m_postConnectThread waits.
    std::condition_variable m_wakeTrigger;

    /// The HTTP2Transport object to which the StateSynchronizer message is sent.
    std::shared_ptr<HTTP2Transport> m_transport;

    /// The context manager that provides application context in the post connect loop.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// Thread upon which @c mainLoop() runs.
    std::thread m_mainLoopThread;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSYNCHRONIZER_H_
