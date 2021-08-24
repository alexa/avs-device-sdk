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
#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTMESSAGEREQUESTOBSERVER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTMESSAGEREQUESTOBSERVER_H_

#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/PromiseFuturePair.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace observer {
namespace test {

class TestMessageRequestObserver : public avsCommon::sdkInterfaces::MessageRequestObserverInterface {
public:
    /*
     * Called when a message request has been processed by AVS.
     */
    void onSendCompleted(MessageRequestObserverInterface::Status status);

    /*
     * Called when an exception is thrown when trying to send a message to AVS.
     */
    void onExceptionReceived(const std::string& exceptionMessage);

    /// A promise that @c MessageRequestObserverInterface::onSendCompleted() will be called  with a @c
    /// MessageRequestObserverInterface::Status value
    PromiseFuturePair<MessageRequestObserverInterface::Status> m_status;

    /// A promise that @c MessageRequestObserverInterface::onExceptionReceived() will be called  with an exception
    /// message
    PromiseFuturePair<std::string> m_exception;
};

}  // namespace test
}  // namespace observer
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTMESSAGEREQUESTOBSERVER_H_
