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

#ifndef ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_POSTCONNECTSYNCHRONIZESTATESENDER_H_
#define ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_POSTCONNECTSYNCHRONIZESTATESENDER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/WaitableMessageRequest.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ContextRequesterInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>

namespace alexaClientSDK {
namespace synchronizeStateSender {

/**
 * A Post connect operation to send the SynchronizeState event.
 */
class PostConnectSynchronizeStateSender
        : public avsCommon::sdkInterfaces::ContextRequesterInterface
        , public avsCommon::sdkInterfaces::PostConnectOperationInterface
        , public std::enable_shared_from_this<PostConnectSynchronizeStateSender> {
public:
    /**
     * Creates a new instance of the @c PostConnectSynchronizeStateSender.
     *
     * @param contextManager The @c ContextManager to request the context from.
     * @return a new instance of the @c PostConnectSynchronizeStateSender.
     */
    static std::shared_ptr<PostConnectSynchronizeStateSender> create(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /// ContextRequesterInterface Methods.
    /// @{
    void onContextAvailable(const std::string& jsonContext) override;
    void onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) override;
    /// @}

    /// PostConnectOperationInterface Methods.
    /// @{
    unsigned int getOperationPriority() override;
    bool performOperation(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) override;
    void abortOperation() override;
    /// @}
private:
    /**
     * Constructor.
     * @param contextManager
     */
    PostConnectSynchronizeStateSender(
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager);

    /**
     * A method to fetch the context and store it in @c m_contextString.
     *
     * @return True if context fetch is successful, else false.
     */
    bool fetchContext();

    /**
     * A thread safe method to check if the operation is stopping.
     * @return True if the operation is stopping, else false.
     */
    bool isStopping();

    /// The @c ContextManagerInterface to request the context from.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// Flag to indicate the PostConnectOperation is stopping.
    bool m_isStopping;

    /// Variable to store context returned from the @c ContextManager
    std::string m_contextString;

    /// Mutex to serialize access to shared variables.
    std::mutex m_mutex;

    /// WaitableMessageRequest used to send the SynchronizeState event.
    std::shared_ptr<avsCommon::avs::WaitableMessageRequest> m_postConnectRequest;

    /// wake trigger to signal when data is ready.
    std::condition_variable m_wakeTrigger;
};

}  // namespace synchronizeStateSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SYNCHRONIZESTATESENDER_INCLUDE_SYNCHRONIZESTATESENDER_POSTCONNECTSYNCHRONIZESTATESENDER_H_
