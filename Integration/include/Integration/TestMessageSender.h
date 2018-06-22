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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTMESSAGESENDER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTMESSAGESENDER_H_

#include <string>
#include <chrono>
#include <deque>
#include <mutex>

#include <ACL/AVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace integration {
namespace test {

class TestMessageSender
        : public avsCommon::sdkInterfaces::MessageSenderInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /// Destructor.
    ~TestMessageSender() = default;

    void sendMessage(std::shared_ptr<avsCommon::avs::MessageRequest> request) override;

    TestMessageSender(
        std::shared_ptr<acl::MessageRouterInterface> messageRouter,
        bool isEnabled,
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> connectionStatusObserver,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> messageObserver);

    class SendParams {
    public:
        enum class Type { SEND, TIMEOUT };
        Type type;
        std::shared_ptr<avsCommon::avs::MessageRequest> request;
    };

    SendParams waitForNext(const std::chrono::seconds duration);

    /**
     * Enable the AVSConnectionManager object to make connections to AVS.  Once enabled, the object will attempt to
     * create a connection to AVS.  If the object is already connected, this function will do nothing.
     */
    void enable();

    /**
     * Disable the AVSConnectionManager object.  If the object is currently connected to AVS, then calling this
     * function will cause the connection to be closed.  If the object is not connected, then calling this function
     * will do nothing.
     */
    void disable();

    /**
     * Returns if the object is enabled for making connections to AVS.
     *
     * @return Whether this Connection object is enabled to make connections.
     */
    bool isEnabled();

    /**
     * This function causes the object, if enabled, to create new connection to AVS.  If the object is already
     * connected, then that connection will be closed and a new one created.  If the object is not connected, but
     * perhaps in the process of waiting for its next connection attempt, then its waiting policy will be reset and
     * it will attempt to create a new connection immediately.
     * If the object is disabled, then this function will do nothing.
     */
    void reconnect();

    /**
     * Set the URL endpoint for the AVS connection.  Calling this function with a new value will cause the
     * current active connection to be closed, and a new one opened to the new endpoint.
     * @param avsEndpoint The URL for the new AVS endpoint.
     */
    void setAVSEndpoint(const std::string& avsEndpoint);

    void addConnectionStatusObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Removes an observer from being notified of connection status changes.
     *
     * @param observer The observer object to remove.
     */
    void removeConnectionStatusObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Adds an observer to be notified of message receptions.
     *
     * @param observer The observer object to add.
     */
    void addMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    /**
     * Removes an observer from being notified of message receptions.
     *
     * @param observer The observer object to remove.
     */
    void removeMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer);

    void doShutdown() override;

    std::shared_ptr<acl::AVSConnectionManager> getConnectionManager() const;

private:
    /// Mutex to protect m_queue.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_wakeTrigger;
    /// Queue of received directives that have not been waited on.
    std::deque<SendParams> m_queue;

    std::shared_ptr<acl::AVSConnectionManager> m_connectionManager;
};

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_TESTMESSAGESENDER_H_
