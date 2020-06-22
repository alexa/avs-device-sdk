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

#ifndef ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCER_H_
#define ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#include "ACL/Transport/PostConnectInterface.h"
#include "ACL/Transport/TransportObserverInterface.h"
#include "ACL/Transport/HTTP2Transport.h"

namespace alexaClientSDK {
namespace acl {

/**
 * Functor for comparing the priority of @c PostConnectOperations.
 */
struct PostConnectOperationPriorityCompare {
    bool operator()(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>& first,
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>& second) const {
        return first->getOperationPriority() < second->getOperationPriority();
    }
};

/**
 * Class that runs a @c PostConnectOperationInterface list in sequence.
 */
class PostConnectSequencer : public PostConnectInterface {
public:
    /// Alias for the custom post connect operations set where items are grouped based on operation priority.
    using PostConnectOperationsSet = std::set<
        std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>,
        PostConnectOperationPriorityCompare>;
    /**
     * Creates a @c PostConnectSequencer instance.
     *
     * @param postConnectOperations The ordered list of @c PostConnectOperationInterfaces.
     * @return a new instance of the @c PostConnectSequencer.
     */
    static std::shared_ptr<PostConnectSequencer> create(const PostConnectOperationsSet& postConnectOperations);

    /**
     * Destructor.
     */
    ~PostConnectSequencer() override;

    /// @name PostConnectInterface methods
    /// @{
    bool doPostConnect(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> postConnectSender,
        std::shared_ptr<PostConnectObserverInterface> postConnectObserver) override;
    void onDisconnect() override;
    ///@}
private:
    /**
     * Constructor.
     *
     * @param postConnectOperations The ordered list of @c PostConnectOperationInterfaces.
     */
    PostConnectSequencer(const PostConnectOperationsSet& postConnectOperations);

    /**
     * Loop to iterate through the @c PostConnectOperationsSet and execute them in sequence.
     *
     * @param postConnectSender The @c MessageSenderInterface to send post connect messages.
     * @param postConnectObserver The @c PostConnectObserverInterface to get notified on successful completion of the
     * post connect action.
     */
    void mainLoop(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> postConnectSender,
        std::shared_ptr<PostConnectObserverInterface> postConnectObserver);

    /**
     * Stop mainLoop().  This method blocks until mainLoop() exits or the operation fails.
     *
     * @return Whether the operation was successful.
     */
    void stop();

    /**
     * A Thread safe method to reset the current post connect operation.
     */
    void resetCurrentOperation();

    /**
     * Thread safe method to check if the @c PostConnectSequencer is stopping.
     * @return
     */
    bool isStopping();

    /// Serializes access to members.
    std::mutex m_mutex;

    /// Current PostConnectOperation being processed.
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> m_currentPostConnectOperation;

    /// Flag indicating a shutdown is initiated.
    bool m_isStopping;

    /// The ordered @c PostConnectOperationInterface list that will be executed in sequence.
    PostConnectOperationsSet m_postConnectOperations;

    /// Mutex to synchronize access to the mainloop thread.
    std::mutex m_mainLoopThreadMutex;

    /// Thread upon which @c mainLoop() runs.
    std::thread m_mainLoopThread;
};

}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_INCLUDE_ACL_TRANSPORT_POSTCONNECTSEQUENCER_H_
