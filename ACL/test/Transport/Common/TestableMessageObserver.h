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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_TESTABLEMESSAGEOBSERVER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_TESTABLEMESSAGEOBSERVER_H_

#include <mutex>
#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>
#include <condition_variable>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * A useful class that allows us to test a Directive being received.
 */
class TestableMessageObserver : public avsCommon::sdkInterfaces::MessageObserverInterface {
public:
    void receive(const std::string& contextId, const std::string& message) override;

    /**
     * A function to wait for a specific directive to be received.
     *
     * @param directiveMessage The text of the directive.
     * @param duraton The maximum time the caller should wait for this to occur.
     * @return Whether the directive was received ok within the timeout.
     */
    bool waitForDirective(const std::string& directiveMessage, const std::chrono::seconds duration);

private:
    /// Mutex for the cv.
    std::mutex m_mutex;
    /// Trigger to wake up waitForNext calls.
    std::condition_variable m_cv;
    /// Directives this observer has received.
    std::vector<std::string> m_receivedDirectives;
};

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_TESTABLEMESSAGEOBSERVER_H_
