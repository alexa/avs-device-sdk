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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEREQUEST_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEREQUEST_H_

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

#include <gmock/gmock.h>

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * A simple mock object to help us test Exceptions being received.
 */
class MockMessageRequest : public avsCommon::avs::MessageRequest {
public:
    /**
     * Constructor.
     */
    MockMessageRequest() : avsCommon::avs::MessageRequest{""} {
    }
    MOCK_METHOD1(exceptionReceived, void(const std::string& exceptionMessage));
    MOCK_METHOD1(sendCompleted, void(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status));
};

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKMESSAGEREQUEST_H_
