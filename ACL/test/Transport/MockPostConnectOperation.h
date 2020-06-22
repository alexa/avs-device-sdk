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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATION_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATION_H_

#include <AVSCommon/SDKInterfaces/PostConnectOperationInterface.h>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

/**
 * Mock class for @c PostConnectOperationInterface.
 */
class MockPostConnectOperation : public avsCommon::sdkInterfaces::PostConnectOperationInterface {
public:
    MOCK_METHOD0(getOperationPriority, unsigned int());
    MOCK_METHOD1(performOperation, bool(const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>&));
    MOCK_METHOD0(abortOperation, void());
};

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATION_H_ */
