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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONINTERFACE_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/*
 * Interface to be implemented by post connect actions that will be executed in sequence by the @c PostConnectSequencer.
 */
class PostConnectOperationInterface {
public:
    /// Operation priority for @c AVS Gateway Verification.
    static constexpr unsigned int VERIFY_GATEWAY_PRIORITY = 50;

    /// Operation priority for @c Publishing Capabilities to AVS.
    static constexpr unsigned int ENDPOINT_DISCOVERY_PRIORITY = 100;

    /// Operation priority for sending @c SynchronizeState event to AVS.
    static constexpr unsigned int SYNCHRONIZE_STATE_PRIORITY = 150;

    /**
     * Destructor.
     */
    virtual ~PostConnectOperationInterface() = default;

    /**
     * Returns the operation priority. The Priority is used to order the sequence of operations in ascending order.
     *
     * @return unsigned int that representing the operation priority.
     */
    virtual unsigned int getOperationPriority() = 0;

    /**
     * Performs the post connect operation. The implementation should ensure that the #performOperation() returns
     * immediately after the #abortOperation() method is called. If #abortOperation() is called before
     * #performOperation(), the method must immediately return with false result.
     *
     * @note This method is not expected to be called twice.
     *
     * @param messageSender - The @c MessageSenderInterface to send post connect message.
     * @return True if the post connect operation is successful, else false.
     */
    virtual bool performOperation(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) = 0;

    /**
     * Aborts an operation that is currently being executed using the performOperation() method.
     *
     * @note This method will be called from a different thread from where the performOperation() is being called from.
     *       It is possible, that the method is called before #performOperation() call is made.
     */
    virtual void abortOperation() = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_POSTCONNECTOPERATIONINTERFACE_H_
