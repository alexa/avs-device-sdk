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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTABLECONSUMER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTABLECONSUMER_H_

#include "ACL/Transport/MessageConsumerInterface.h"
#include <AVSCommon/SDKInterfaces/MessageObserverInterface.h>

#include <iostream>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * Simple implementation of the ACL MessageConsumerInterface, which we can use to satisfy other ACL constructors.
 */
class TestableConsumer : public acl::MessageConsumerInterface {
public:
    /**
     * Constructor.
     *
     * @param observer An observer which will receive any Messages this object receives through consumeMessage.
     */
    void setMessageObserver(std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
        m_messageObserver = observer;
    }

    void consumeMessage(const std::string& contextId, const std::string& message) override {
        if (m_messageObserver) {
            m_messageObserver->receive(contextId, message);
        }
    }

private:
    /// The observer object which Messages should be propagated to once consumed by this object.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> m_messageObserver;
};

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_TESTABLECONSUMER_H_
